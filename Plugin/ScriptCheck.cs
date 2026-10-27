using System.Text.RegularExpressions;
using Mutagen.Bethesda.Plugins.Records;
using Mutagen.Bethesda.Skyrim;

namespace LazyCamping.PluginBuilder;

/// <summary>
/// Compares the script properties set in the built plugin with the
/// properties declared in the .psc sources, so a renamed or misspelt
/// property is caught before it fails silently in game.
/// </summary>
public static class ScriptCheck
{
    static readonly HashSet<string> ValueTypes = new(StringComparer.OrdinalIgnoreCase) { "int", "float", "bool", "string" };
    static readonly Regex PropertyLine = new(@"^\s*(\w+(?:\[\])?)\s+Property\s+(\w+)(.*)$", RegexOptions.IgnoreCase);

    public static bool Run(string pluginPath, string scriptDir)
    {
        using var plugin = SkyrimMod.CreateFromBinaryOverlay(pluginPath, SkyrimRelease.SkyrimSE);

        // Every script attached anywhere, with the property names set on it.
        var attached = new Dictionary<string, HashSet<string>>(StringComparer.OrdinalIgnoreCase);
        void Collect(IEnumerable<IScriptEntryGetter> scripts)
        {
            foreach (var script in scripts)
            {
                var names = attached.TryGetValue(script.Name, out var set) ? set : attached[script.Name] = new(StringComparer.OrdinalIgnoreCase);
                foreach (var property in script.Properties)
                {
                    names.Add(property.Name);
                }
            }
        }
        foreach (var record in plugin.EnumerateMajorRecords())
        {
            if (record is IHaveVirtualMachineAdapterGetter { VirtualMachineAdapter: { } adapter })
            {
                Collect(adapter.Scripts);
                if (adapter is IQuestAdapterGetter quest)
                {
                    foreach (var alias in quest.Aliases)
                    {
                        Collect(alias.Scripts);
                    }
                }
            }
        }

        var ok = true;
        foreach (var (script, set) in attached.OrderBy(p => p.Key))
        {
            var source = Path.Combine(scriptDir, script + ".psc");
            if (!File.Exists(source))
            {
                Console.Error.WriteLine($"  {script}: attached in the plugin, but {source} doesn't exist");
                ok = false;
                continue;
            }
            var declared = Declared(source);
            foreach (var name in set.Where(n => !declared.ContainsKey(n)))
            {
                Console.Error.WriteLine($"  {script}: plugin sets \"{name}\", which the script doesn't declare");
                ok = false;
            }
            foreach (var name in declared.Where(d => d.Value && !set.Contains(d.Key)).Select(d => d.Key))
            {
                Console.WriteLine($"  {script}: \"{name}\" is left empty (fine if it's optional)");
            }
        }
        Console.WriteLine(ok ? $"Script properties match the sources ({attached.Count} scripts)." : "Script properties don't match the sources.");
        return ok;
    }

    /// <summary>Declared properties, mapped to whether they're objects that need filling in.</summary>
    public static Dictionary<string, bool> Declared(string path)
    {
        var result = new Dictionary<string, bool>(StringComparer.OrdinalIgnoreCase);
        foreach (var line in File.ReadLines(path))
        {
            var match = PropertyLine.Match(line);
            if (!match.Success)
            {
                continue;
            }
            var rest = match.Groups[3].Value;
            if (rest.Contains("AutoReadOnly", StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }
            var type = match.Groups[1].Value.TrimEnd('[', ']');
            var needsFilling = !ValueTypes.Contains(type) && !rest.Contains("Hidden", StringComparison.OrdinalIgnoreCase);
            result[match.Groups[2].Value] = needsFilling;
        }
        return result;
    }
}
