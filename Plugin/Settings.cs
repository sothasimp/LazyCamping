using System.Text.Json;

namespace LazyCamping.PluginBuilder;

public sealed class Settings
{
    public VanillaIds Vanilla { get; set; } = new();
    public ModelPaths Models { get; set; } = new();
    public TentTextureSettings TentTextures { get; set; } = new();
    public KitSettings Kits { get; set; } = new();

    public static Settings Load(string path) => Read<Settings>(path);

    internal static T Read<T>(string path) where T : new()
    {
        var options = new JsonSerializerOptions
        {
            PropertyNameCaseInsensitive = true,
            ReadCommentHandling = JsonCommentHandling.Skip,
            AllowTrailingCommas = true,
        };
        return JsonSerializer.Deserialize<T>(File.ReadAllText(path), options) ?? new T();
    }
}

public sealed class VanillaIds
{
    public string Campfire { get; set; } = "";
    public string CampfireLight { get; set; } = "";
    public string CampfireUnlit { get; set; } = "";
    public string ChoppingBlock { get; set; } = "";
    public string UnpackSound { get; set; } = "";
    public string BuildSound { get; set; } = "";
    public string Bedroll { get; set; } = "";
    public string SitCrossLegged { get; set; } = "";
    public string VoiceEquipType { get; set; } = "";
    public string Firewood { get; set; } = "";
    public string LocTypeCity { get; set; } = "";
    public string LocTypeTown { get; set; } = "";
    public string PartnerFaction { get; set; } = "";
}

public sealed class ModelPaths
{
    public string BedrollKit { get; set; } = "";
    public string SmallTentKit { get; set; } = "";
    public string LargeTentKit { get; set; } = "";
    public string SmallTent { get; set; } = "";
    public string LargeTent { get; set; } = "";
    public string PlayerSeat { get; set; } = "";
    public string BedrollSeat { get; set; } = "";
    public string Storage { get; set; } = "";
}

public sealed class TentTextureSettings
{
    public string SmallShape { get; set; } = "";
    public string LargeShape { get; set; } = "";
    public string Path { get; set; } = "";

    /// <summary>What {Colour} becomes in Path, for colours whose files are named differently.</summary>
    public Dictionary<string, string> FileColours { get; set; } = new(StringComparer.OrdinalIgnoreCase);
}

public sealed class KitSettings
{
    public KitInfo Bedroll { get; set; } = new();
    public KitInfo SmallTent { get; set; } = new();
    public KitInfo LargeTent { get; set; } = new();
}

public sealed class KitInfo
{
    public string Name { get; set; } = "";
    public float Weight { get; set; }
    public uint Value { get; set; }
}

/// <summary>Clutter sets from clutter.json.</summary>
public sealed class ClutterSets
{
    public List<ClutterItem> Fire { get; set; } = new();
    public List<ClutterItem> SmallTent { get; set; } = new();
    public List<ClutterItem> LargeTent { get; set; } = new();
    public List<ClutterItem> RingBedroll { get; set; } = new();

    public static ClutterSets Load(string path) => Settings.Read<ClutterSets>(path);
}

public sealed class ClutterItem
{
    public string Base { get; set; } = "";
    public float[] Offset { get; set; } = [0, 0, 0];
    public float[] Rotation { get; set; } = [0, 0, 0];
    public float Scale { get; set; } = 1;
    public bool Storage { get; set; }
}
