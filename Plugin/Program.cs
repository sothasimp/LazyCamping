using LazyCamping.PluginBuilder;
using Mutagen.Bethesda;
using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Plugins.Cache;
using Mutagen.Bethesda.Plugins.Records;
using Mutagen.Bethesda.Skyrim;

// Builds the starting LazyCamping.esp once; after that the plugin is edited in
// the Creation Kit, and --verify checks it against the scripts.
//
// Usage:
//   build:        dotnet run --project Plugin -- --master "<Skyrim>/Data/Skyrim.esm" [--out LazyCamping.esp] [--force]
//   verify:       dotnet run --project Plugin -- --verify <LazyCamping.esp> --scripts Scripts/Source [--master <Skyrim.esm>]
//   search:       dotnet run --project Plugin -- --master <Skyrim.esm> --find <text>
//   test master:  dotnet run --project Plugin -- --make-test-master <path>
var options = ParseArgs(args);
var here = AppContext.BaseDirectory;

try
{
    if (options.TryGetValue("make-test-master", out var testMaster))
    {
        var settingsForTest = Settings.Load(options.GetValueOrDefault("settings") ?? Path.Combine(here, "settings.json"));
        TestMaster.Write(testMaster, settingsForTest);
        Console.WriteLine($"Wrote stand-in master {testMaster}");
        return 0;
    }

    if (options.TryGetValue("verify", out var verifyPath))
    {
        // Links can only be checked with the master; script properties only need the plugin.
        var linksOk = !options.TryGetValue("master", out var verifyMaster) || Verify(verifyPath, verifyMaster) == 0;
        var scriptsOk = !options.TryGetValue("scripts", out var scriptDir) || ScriptCheck.Run(verifyPath, scriptDir);
        return linksOk && scriptsOk ? 0 : 1;
    }

    if (!options.TryGetValue("master", out var masterPath))
    {
        Console.Error.WriteLine("Pass --master with the path to Skyrim.esm (in the game's Data folder).");
        return 2;
    }

    if (options.TryGetValue("find", out var findText))
    {
        using var searched = SkyrimMod.CreateFromBinaryOverlay(masterPath, SkyrimRelease.SkyrimSE);
        var found = Builder.Search(searched, findText).ToList();
        Console.WriteLine(found.Count == 0 ? $"No EditorID contains \"{findText}\"." : string.Join(Environment.NewLine, found));
        return 0;
    }

    var settings = Settings.Load(options.GetValueOrDefault("settings") ?? Path.Combine(here, "settings.json"));
    var dressing = DressingSets.Load(options.GetValueOrDefault("dressing") ?? Path.Combine(here, "dressing.json"));
    var outPath = options.GetValueOrDefault("out") ?? "LazyCamping.esp";
    if (File.Exists(outPath) && !options.ContainsKey("force"))
    {
        Console.Error.WriteLine($"{outPath} already exists. It may have Creation Kit edits, so it won't be overwritten; pass --force to replace it.");
        return 1;
    }

    using var skyrim = SkyrimMod.CreateFromBinaryOverlay(masterPath, SkyrimRelease.SkyrimSE);
    var builder = new Builder(skyrim, settings, dressing);
    var mod = builder.Build();
    if (builder.Problems.Count > 0)
    {
        Console.Error.WriteLine("Couldn't build LazyCamping.esp. Fix these in settings.json or dressing.json:");
        foreach (var problem in builder.Problems)
        {
            Console.Error.WriteLine($"  - {problem}");
        }
        return 1;
    }

    mod.WriteToBinary(outPath);
    Console.WriteLine($"Wrote {outPath}: {mod.EnumerateMajorRecords().Count()} records.");
    return 0;
}
catch (Exception e)
{
    Console.Error.WriteLine(e);
    return 1;
}

// Reads the built plugin back and checks that every link points at a record
// in it or in the master.
static int Verify(string pluginPath, string masterPath)
{
    using var master = SkyrimMod.CreateFromBinaryOverlay(masterPath, SkyrimRelease.SkyrimSE);
    using var plugin = SkyrimMod.CreateFromBinaryOverlay(pluginPath, SkyrimRelease.SkyrimSE);
    var cache = new ISkyrimModGetter[] { master, plugin }.ToImmutableLinkCache();

    var broken = 0;
    foreach (var record in plugin.EnumerateMajorRecords())
    {
        foreach (var link in record.EnumerateFormLinks())
        {
            if (link.IsNull || link.FormKey == Builder.PlayerRefKey)
            {
                continue; // PlayerRef is a placed actor in the real Skyrim.esm.
            }
            if (!cache.TryResolve(link.FormKey, link.Type, out _))
            {
                Console.Error.WriteLine($"  {record.EditorID ?? record.FormKey.ToString()} -> missing {link.FormKey} ({link.Type.Name})");
                broken++;
            }
        }
    }

    var counts = plugin.EnumerateMajorRecords()
        .GroupBy(r => r.GetType().Name.Replace("BinaryOverlay", ""))
        .OrderBy(g => g.Key)
        .Select(g => $"{g.Key} {g.Count()}");
    Console.WriteLine($"Records: {string.Join(", ", counts)}");
    Console.WriteLine(broken == 0 ? "All links resolve." : $"{broken} broken links.");
    return broken == 0 ? 0 : 1;
}

static Dictionary<string, string> ParseArgs(string[] args)
{
    var result = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
    for (var i = 0; i < args.Length; i++)
    {
        if (args[i].StartsWith("--"))
        {
            var key = args[i][2..];
            var value = i + 1 < args.Length && !args[i + 1].StartsWith("--") ? args[++i] : "";
            result[key] = value;
        }
    }
    return result;
}
