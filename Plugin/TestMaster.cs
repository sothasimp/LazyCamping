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
        // One sound of each kind the builder accepts.
        if (!string.IsNullOrWhiteSpace(v.UnpackSound))
        {
            mod.SoundDescriptors.AddNew(v.UnpackSound);
        }
        if (!string.IsNullOrWhiteSpace(v.BuildSound))
        {
            mod.SoundMarkers.AddNew(v.BuildSound);
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

        // Package templates with the kinds of inputs the builder rewires.
        var sleep = mod.Packages.AddNew(v.SleepTemplate);
        sleep.Type = Package.Types.PackageTemplate;
        sleep.Data[0] = new PackageDataLocation { Name = "Location", Location = new LocationTargetRadius() };
        sleep.Data[1] = new PackageDataBool { Name = "Lock Doors" };

        var sitTemplate = mod.Packages.AddNew(v.SitTemplate);
        sitTemplate.Type = Package.Types.PackageTemplate;
        sitTemplate.Data[0] = new PackageDataLocation { Name = "Location", Location = new LocationTargetRadius() };
        sitTemplate.Data[1] = new PackageDataTarget { Name = "Target", Target = new PackageTargetSelf() };

        // The real master's low FormIDs (MapMarker, XMarkerHeading) are normally reserved.
        mod.WriteToBinary(path, BinaryWriteParameters.Default with
        {
            LowerRangeDisallowedHandler = ALowerRangeDisallowedHandlerOption.NoCheck,
        });
    }
}
