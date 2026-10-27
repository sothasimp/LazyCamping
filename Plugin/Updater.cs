using Mutagen.Bethesda;
using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Plugins.Records;
using Mutagen.Bethesda.Skyrim;
using Noggog;

namespace LazyCamping.PluginBuilder;

/// <summary>
/// Brings a LazyCamping.esp that has been edited in the Creation Kit up to
/// date with a fresh build, without undoing the edits. Records are matched by
/// EditorID. It only:
/// <list type="bullet">
/// <item>adds records the build makes and the plugin lacks (holding cell
/// references only if they have an EditorID, so hand-placed clutter isn't
/// duplicated), with links pointed at the plugin's own records;</item>
/// <item>on records the plugin already has, attaches the LZC_ scripts the
/// build attaches, removes LZC_ scripts it no longer attaches, adds missing
/// properties to them, and drops properties their .psc no longer declares.</item>
/// </list>
/// Everything else (models, bounds, texture sets, packages, aliases,
/// non-LZC scripts) is left as it is.
/// </summary>
public sealed class Updater(SkyrimMod plugin, SkyrimMod fresh, string? scriptDir)
{
    const string OurPrefix = "LZC_";

    public List<string> Report { get; } = [];

    /// <summary>The plugin's own records whose data the update changed (not the ones it added).</summary>
    public HashSet<FormKey> Changed { get; } = [];

    public void Run()
    {
        // New FormIDs must come after every one the plugin uses, whatever
        // the header says.
        var highest = plugin.EnumerateMajorRecords().Where(r => r.FormKey.ModKey == plugin.ModKey)
            .Select(r => r.FormKey.ID).DefaultIfEmpty(0x7FFu).Max();
        if (plugin.ModHeader.Stats.NextFormID <= highest)
        {
            plugin.ModHeader.Stats.NextFormID = highest + 1;
        }

        var byEditorId = plugin.EnumerateMajorRecords()
            .Where(r => r.EditorID is not null)
            .GroupBy(r => r.EditorID!, StringComparer.OrdinalIgnoreCase)
            .ToDictionary(g => g.Key, g => g.First(), StringComparer.OrdinalIgnoreCase);

        // Every fresh record maps to the plugin's record with its EditorID, or
        // to a new FormKey in the plugin.
        var remap = new Dictionary<FormKey, FormKey>();
        var missing = new List<IMajorRecord>();
        foreach (var record in fresh.EnumerateMajorRecords())
        {
            if (record.EditorID is { } id && byEditorId.TryGetValue(id, out var existing))
            {
                remap[record.FormKey] = existing.FormKey;
            }
            else if (record.EditorID is not null)
            {
                remap[record.FormKey] = plugin.GetNextFormKey();
                missing.Add(record);
            }
        }

        // Unnamed references (clutter) are only added when a script property
        // that lists them is new to the plugin, i.e. for a clutter set it
        // doesn't have yet; hand-placed clutter is never duplicated.
        var freshRecords = fresh.EnumerateMajorRecords().ToDictionary(r => r.FormKey);
        foreach (var record in fresh.EnumerateMajorRecords())
        {
            if (record is not IHaveVirtualMachineAdapterGetter { VirtualMachineAdapter: { } wanted } ||
                record.EditorID is not { } id || !byEditorId.TryGetValue(id, out var existing) ||
                existing is not IHaveVirtualMachineAdapterGetter { VirtualMachineAdapter: { } have })
            {
                continue;
            }
            foreach (var script in wanted.Scripts)
            {
                var haveScript = have.Scripts.FirstOrDefault(s => string.Equals(s.Name, script.Name, StringComparison.OrdinalIgnoreCase));
                foreach (var property in script.Properties)
                {
                    if (haveScript is not null && haveScript.Properties.Any(p => string.Equals(p.Name, property.Name, StringComparison.OrdinalIgnoreCase)))
                    {
                        continue;
                    }
                    foreach (var link in property.EnumerateFormLinks())
                    {
                        if (!remap.ContainsKey(link.FormKey) && freshRecords.TryGetValue(link.FormKey, out var unnamed))
                        {
                            remap[link.FormKey] = plugin.GetNextFormKey();
                            missing.Add(unnamed);
                        }
                    }
                }
            }
        }

        foreach (var record in missing)
        {
            Add(record, remap);
        }

        foreach (var record in fresh.EnumerateMajorRecords())
        {
            if (record is IHaveVirtualMachineAdapterGetter { VirtualMachineAdapter: { } wanted } &&
                record.EditorID is { } id && byEditorId.TryGetValue(id, out var existing) &&
                existing is IHaveVirtualMachineAdapter target)
            {
                var before = Report.Count;
                UpdateScripts(id, target, wanted, remap);
                if (Report.Count > before)
                {
                    Changed.Add(existing.FormKey);
                }
            }
        }
    }

    /// <summary>Links into the plugin itself that don't point at one of its records.</summary>
    public IEnumerable<string> DanglingLinks()
    {
        var own = plugin.EnumerateMajorRecords().Select(r => r.FormKey).ToHashSet();
        foreach (var record in plugin.EnumerateMajorRecords())
        {
            foreach (var link in record.EnumerateFormLinks())
            {
                if (link.FormKey.ModKey == plugin.ModKey && !own.Contains(link.FormKey))
                {
                    yield return $"{record.EditorID ?? record.FormKey.ToString()} -> {link.FormKey}";
                }
            }
        }
    }

    static string Describe(MajorRecord record) =>
        record.EditorID ?? (record is PlacedObject placed ? $"(clutter, base {placed.Base.FormKey})" : record.FormKey.ToString());

    void Add(IMajorRecord record, Dictionary<FormKey, FormKey> remap)
    {
        var copy = (MajorRecord)record.Duplicate(remap[record.FormKey]);
        copy.RemapLinks(remap);
        if (copy is PlacedObject placed)
        {
            var cell = plugin.Cells.Records.SelectMany(b => b.SubBlocks).SelectMany(s => s.Cells)
                .FirstOrDefault(c => string.Equals(c.EditorID, "LZC_HoldingCell", StringComparison.OrdinalIgnoreCase));
            if (cell is null)
            {
                Report.Add($"skipped {record.EditorID}: no LZC_HoldingCell to put it in");
                return;
            }
            cell.Persistent.Add(placed);
        }
        else if (copy is Cell)
        {
            Report.Add($"skipped {record.EditorID}: adding a whole cell isn't supported");
            return;
        }
        else
        {
            ((IMod)plugin).GetTopLevelGroup(copy.GetType()).AddUntyped(copy);
        }
        Report.Add($"added {copy.GetType().Name} {Describe(copy)}");
    }

    void UpdateScripts(string owner, IHaveVirtualMachineAdapter target, IAVirtualMachineAdapterGetter wanted, Dictionary<FormKey, FormKey> remap)
    {
        if (target.VirtualMachineAdapter is null)
        {
            // The setter is on each record type, not the interface.
            var adapter = wanted.DeepCopy();
            adapter.RemapLinks(remap);
            target.GetType().GetProperty("VirtualMachineAdapter")!.SetValue(target, adapter);
            Report.Add($"{owner}: attached {string.Join(", ", wanted.Scripts.Select(s => s.Name))}");
            return;
        }
        Merge(owner, target.VirtualMachineAdapter.Scripts, wanted.Scripts, remap);

        if (target.VirtualMachineAdapter is QuestAdapter quest && wanted is IQuestAdapterGetter wantedQuest)
        {
            foreach (var wantedAlias in wantedQuest.Aliases)
            {
                var alias = quest.Aliases.FirstOrDefault(a => a.Property.Alias == wantedAlias.Property.Alias);
                if (alias is null)
                {
                    var copy = wantedAlias.DeepCopy();
                    copy.RemapLinks(remap);
                    quest.Aliases.Add(copy);
                    Report.Add($"{owner}: alias {wantedAlias.Property.Alias} gets {string.Join(", ", wantedAlias.Scripts.Select(s => s.Name))}");
                    continue;
                }
                Merge($"{owner} alias {alias.Property.Alias}", alias.Scripts, wantedAlias.Scripts, remap);
            }
            // Aliases whose LZC_ scripts the build no longer attaches.
            foreach (var alias in quest.Aliases)
            {
                if (!wantedQuest.Aliases.Any(a => a.Property.Alias == alias.Property.Alias))
                {
                    Merge($"{owner} alias {alias.Property.Alias}", alias.Scripts, [], remap);
                }
            }
        }
    }

    void Merge(string owner, IList<ScriptEntry> scripts, IEnumerable<IScriptEntryGetter> wanted, Dictionary<FormKey, FormKey> remap)
    {
        var wantedList = wanted.ToList();
        for (var i = scripts.Count - 1; i >= 0; i--)
        {
            var name = scripts[i].Name;
            if (name.StartsWith(OurPrefix, StringComparison.OrdinalIgnoreCase) &&
                !wantedList.Any(w => string.Equals(w.Name, name, StringComparison.OrdinalIgnoreCase)))
            {
                scripts.RemoveAt(i);
                Report.Add($"{owner}: removed {name}");
            }
        }
        foreach (var want in wantedList)
        {
            var script = scripts.FirstOrDefault(s => string.Equals(s.Name, want.Name, StringComparison.OrdinalIgnoreCase));
            if (script is null)
            {
                script = want.DeepCopy();
                script.RemapLinks(remap);
                scripts.Add(script);
                Report.Add($"{owner}: attached {want.Name}");
                continue;
            }
            foreach (var property in want.Properties)
            {
                if (!script.Properties.Any(p => string.Equals(p.Name, property.Name, StringComparison.OrdinalIgnoreCase)))
                {
                    var copy = (ScriptProperty)property.DeepCopy();
                    copy.RemapLinks(remap);
                    script.Properties.Add(copy);
                    Report.Add($"{owner}: {want.Name} gets {property.Name}");
                }
            }
        }
        if (scriptDir is null)
        {
            return;
        }
        foreach (var script in scripts.Where(s => s.Name.StartsWith(OurPrefix, StringComparison.OrdinalIgnoreCase)))
        {
            var source = Path.Combine(scriptDir, script.Name + ".psc");
            if (!File.Exists(source))
            {
                continue;
            }
            var declared = ScriptCheck.Declared(source);
            foreach (var property in script.Properties.Where(p => !declared.ContainsKey(p.Name)).ToList())
            {
                script.Properties.Remove(property);
                Report.Add($"{owner}: {script.Name} drops {property.Name}");
            }
        }
    }
}
