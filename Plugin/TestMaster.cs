using Mutagen.Bethesda;
using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Plugins.Binary.Parameters;
using Mutagen.Bethesda.Skyrim;

namespace LazyCamping.PluginBuilder;

/// <summary>
/// Writes a stand-in Skyrim.esm holding just the records the builder looks
/// up, so the builder can run where the game isn't installed (such as CI).
/// The records only have the shape the builder relies on; they are not the
/// real game data, and a plugin built against them is only for testing.
/// </summary>
public static class TestMaster
{
    public static void Write(string path, Settings settings)
    {
        var mod = new SkyrimMod(Builder.SkyrimKey, SkyrimRelease.SkyrimSE);
        mod.ModHeader.Flags |= SkyrimModHeader.HeaderFlag.Master;
        var v = settings.Vanilla;

        mod.Statics.Add(new Static(Builder.MapMarkerKey, SkyrimRelease.SkyrimSE) { EditorID = "MapMarker" });
        mod.Statics.Add(new Static(Builder.XMarkerHeadingKey, SkyrimRelease.SkyrimSE) { EditorID = "XMarkerHeading" });
        mod.Activators.AddNew(v.Campfire); // Vanilla campfires are activators.
        mod.Lights.AddNew(v.CampfireLight);
        mod.Statics.AddNew(v.CampfireUnlit).Model = new Model { File = @"Clutter\Campfire01Off.nif" };
        mod.Furniture.AddNew(v.ChoppingBlock);
        mod.Furniture.AddNew(v.CookingPot);
        mod.Statics.AddNew(v.TubWater);
        foreach (var sound in new[] { v.UnpackSound }.Where(s => !string.IsNullOrWhiteSpace(s)))
        {
            mod.SoundDescriptors.AddNew(sound);
        }
        mod.Furniture.AddNew(v.Bedroll);
        var sit = mod.Furniture.AddNew(v.SitCrossLegged);
        sit.Markers = [new FurnitureMarker { Enabled = true }];
        mod.EquipTypes.AddNew(v.VoiceEquipType);
        mod.MiscItems.AddNew(v.Firewood);
        mod.Keywords.AddNew(v.LocTypeCity);
        mod.Keywords.AddNew(v.LocTypeTown);
        if (!string.IsNullOrWhiteSpace(v.PartnerFaction))
        {
            mod.Factions.AddNew(v.PartnerFaction);
        }

        // The real master's low FormIDs (MapMarker, XMarkerHeading) are normally reserved.
        mod.WriteToBinary(path, BinaryWriteParameters.Default with
        {
            LowerRangeDisallowedHandler = ALowerRangeDisallowedHandlerOption.NoCheck,
        });
    }
}
