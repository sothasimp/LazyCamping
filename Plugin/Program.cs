using LazyCamping.PluginBuilder;
using Mutagen.Bethesda;
using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Plugins.Cache;
using Mutagen.Bethesda.Plugins.Records;
using Mutagen.Bethesda.Skyrim;

// Usage:
//   build:        dotnet run --project Plugin -- --master "<Skyrim>/Data/Skyrim.esm" [--out LazyCamping.esp]
//   test master:  dotnet run --project Plugin -- --make-test-master <path>
//   verify:       dotnet run --project Plugin -- --verify <LazyCamping.esp> --master <Skyrim.esm> [--scripts Scripts/Source]
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

    if (!options.TryGetValue("master", out var masterPath))
    {
        Console.Error.WriteLine("Pass --master with the path to Skyrim.esm (in the game's Data folder).");
        return 2;
    }

    if (options.TryGetValue("verify", out var verifyPath))
    {
        var linksOk = Verify(verifyPath, masterPath) == 0;
        var scriptsOk = !options.TryGetValue("scripts", out var scriptDir) || ScriptCheck.Run(verifyPath, scriptDir);
        return linksOk && scriptsOk ? 0 : 1;
    }

    var settings = Settings.Load(options.GetValueOrDefault("settings") ?? Path.Combine(here, "settings.json"));
    var dressing = DressingSets.Load(options.GetValueOrDefault("dressing") ?? Path.Combine(here, "dressing.json"));
    var outPath = options.GetValueOrDefault("out") ?? "LazyCamping.esp";

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
