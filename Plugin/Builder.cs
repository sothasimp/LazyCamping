using Mutagen.Bethesda;
using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Plugins.Records;
using Mutagen.Bethesda.Skyrim;
using Noggog;
using Activator = Mutagen.Bethesda.Skyrim.Activator;

namespace LazyCamping.PluginBuilder;

/// <summary>
/// Builds LazyCamping.esp: every record the scripts in Scripts/Source expect,
/// with the same LZC_ EditorIDs and script properties. Vanilla records are
/// looked up in Skyrim.esm by EditorID (see settings.json).
/// </summary>
public sealed class Builder
{
    public static readonly ModKey OutputKey = ModKey.FromFileName("LazyCamping.esp");
    public static readonly ModKey SkyrimKey = ModKey.FromFileName("Skyrim.esm");

    // Fixed Skyrim.esm records.
    public static readonly FormKey PlayerRefKey = new(SkyrimKey, 0x000014);
    public static readonly FormKey MapMarkerKey = new(SkyrimKey, 0x000010);
    public static readonly FormKey XMarkerHeadingKey = new(SkyrimKey, 0x000034);

    public static readonly string[] Colours = ["Brown", "Blue", "Green", "Red", "White"];
    const int MaxFollowers = 3;
    const short ScriptVersion = 5;
    const ushort ScriptObjectFormat = 2;
    const int PersistentFlag = 0x400;   // reference record flag: persistent

    readonly ISkyrimModGetter skyrim;
    readonly Settings settings;
    readonly ClutterSets clutter;
    readonly SkyrimMod mod;
    readonly List<string> problems = [];

    public Builder(ISkyrimModGetter skyrim, Settings settings, ClutterSets clutter)
    {
        this.skyrim = skyrim;
        this.settings = settings;
        this.clutter = clutter;
        mod = new SkyrimMod(OutputKey, SkyrimRelease.SkyrimSE);
    }

    /// <summary>Lookups that failed, with close matches. Empty when the build succeeded.</summary>
    public IReadOnlyList<string> Problems => problems;

    public SkyrimMod Build()
    {
        // Vanilla records.
        var campfire = Find<IPlaceableObjectGetter>(settings.Vanilla.Campfire, "campfire");
        var campfireLight = Find<ILightGetter>(settings.Vanilla.CampfireLight, "fire");
        var campfireUnlit = Find<IPlaceableObjectGetter>(settings.Vanilla.CampfireUnlit, "campfire");
        var choppingBlock = Find<IFurnitureGetter>(settings.Vanilla.ChoppingBlock, "chopping");
        var cookingPot = Find<IFurnitureGetter>(settings.Vanilla.CookingPot, "cooking");
        var bedroll = Find<IFurnitureGetter>(settings.Vanilla.Bedroll, "bedroll");
        var sitMarker = Find<IFurnitureGetter>(settings.Vanilla.SitCrossLegged, "crossleg");
        var voice = Find<IEquipTypeGetter>(settings.Vanilla.VoiceEquipType, "voice");
        var firewood = Find<IMiscItemGetter>(settings.Vanilla.Firewood, "firewood");
        var locTypeCity = Find<IKeywordGetter>(settings.Vanilla.LocTypeCity, "loctype");
        var locTypeTown = Find<IKeywordGetter>(settings.Vanilla.LocTypeTown, "loctype");
        var partnerFaction = Find<IFactionGetter>(settings.Vanilla.PartnerFaction, "married");
        if (problems.Count > 0 || sitMarker is null)
        {
            return mod;
        }

        // Quests first, so other records can point at them.
        var campQuest = mod.Quests.AddNew("LZC_CampsiteQuest");
        var followersQuest = mod.Quests.AddNew("LZC_CampFollowersQuest");
        // Alias packages compete by quest priority: outrank the vanilla
        // follower quest's follow package, or followers never sit or sleep.
        followersQuest.Priority = 90;

        var activity = new GlobalShort(mod.GetNextFormKey(), SkyrimRelease.SkyrimSE)
        {
            EditorID = "LZC_CampActivity",
            Data = 0,
        };
        mod.Globals.Add(activity);

        // Kits.
        var bedrollKit = MiscItem("LZC_BedrollKit", settings.Kits.Bedroll, "", settings.Models.BedrollKit);
        var smallKits = Colours.Select(c => MiscItem($"LZC_TentKitSmall{c}", settings.Kits.SmallTent, c, settings.Models.SmallTentKit)).ToList();
        var largeKits = Colours.Select(c => MiscItem($"LZC_TentKitLarge{c}", settings.Kits.LargeTent, c, settings.Models.LargeTentKit)).ToList();

        // Tents: one Activator per size and colour.
        var smallTents = Colours.Select(c => Tent("Small", c, settings.Models.SmallTent, settings.TentTextures.SmallShape, 85, 85, campQuest)).ToList();
        var largeTents = Colours.Select(c => Tent("Large", c, settings.Models.LargeTent, settings.TentTextures.LargeShape, 150, 100, campQuest)).ToList();

        // Seats: copies of the vanilla cross-legged marker with our own models.
        var playerSeat = Seat("LZC_SitMarkerPlayer", sitMarker, settings.Models.PlayerSeat);
        var bedrollSeat = Seat("LZC_BedrollSitMarker", sitMarker, settings.Models.BedrollSeat);

        // The storage backpack: a container that never respawns.
        var storageContainer = mod.Containers.AddNew("LZC_CampStorageContainer");
        storageContainer.Name = "Backpack";
        storageContainer.Model = new Model { File = settings.Models.Storage };
        storageContainer.ObjectBounds = Bounds(25, 15, 40);

        // The unlit firepit for the firewood mechanic: the vanilla model, with a script.
        Activator? firepit = null;
        if (campfireUnlit is not null)
        {
            firepit = mod.Activators.AddNew("LZC_Firepit");
            firepit.Name = "Firepit";
            firepit.Model = (campfireUnlit as IModeledGetter)?.Model?.DeepCopy();
            firepit.ObjectBounds = (campfireUnlit as IObjectBoundedGetter)?.ObjectBounds.DeepCopy() ?? Bounds(50, 50, 30);
            firepit.VirtualMachineAdapter = Scripts(Script("LZC_FirepitScript", Link("CampQuest", campQuest)));
        }

        // Cooking at the camp's (vanilla) fire: an invisible activator box,
        // placed in the Creation Kit, moves onto the fire and shows "Cook";
        // activating it activates a hidden cooking station (a copy of the
        // vanilla cooking pot with an empty model, so there's nothing to see
        // and no animation). This is the box's base.
        var cookActivator = mod.Activators.AddNew("LZC_CookActivator");
        cookActivator.Name = "Campfire";
        cookActivator.ActivateTextOverride = "Cook";
        cookActivator.VirtualMachineAdapter = Scripts(Script("LZC_CampfireScript", Link("CampQuest", campQuest)));
        Furniture? cookingStation = null;
        if (cookingPot is not null)
        {
            cookingStation = mod.Furniture.DuplicateInAsNewRecord(cookingPot, "LZC_CookingStation");
            cookingStation.Name = "Campfire";
            cookingStation.Model = new Model { File = settings.Models.CookingStation };
        }

        // The bathtub: a static with the tub mesh. Its water, steps and so on
        // are the tub clutter set, which can use the clothes basket: activating
        // it undresses the player.
        Static? tub = null;
        if (!string.IsNullOrWhiteSpace(settings.Models.Tub))
        {
            tub = mod.Statics.AddNew("LZC_Tub");
            tub.Model = new Model { File = settings.Models.Tub };
            tub.ObjectBounds = Bounds(50, 35, 40);
        }
        var basket = mod.Activators.AddNew("LZC_ClothesBasket");
        basket.Name = "Clothes Basket";
        basket.ActivateTextOverride = "Undress";
        if (!string.IsNullOrWhiteSpace(settings.Models.ClothesBasket))
        {
            basket.Model = new Model { File = settings.Models.ClothesBasket };
        }
        basket.ObjectBounds = Bounds(20, 20, 20);
        basket.VirtualMachineAdapter = Scripts(Script("LZC_ClothesBasketScript", Link("PlayerRef", PlayerRefKey)));

        var unpackSound = Sound("LZC_UnpackSound", settings.Vanilla.UnpackSound, "clothing");

        var packUpMsg = mod.Messages.AddNew("LZC_PackUpConfirmMsg");
        packUpMsg.Description = "Pack up your camp?";
        packUpMsg.Flags = Message.Flag.MessageBox;
        packUpMsg.MenuButtons.Add(new MessageButton { Text = "Yes" });
        packUpMsg.MenuButtons.Add(new MessageButton { Text = "No" });

        // The power.
        var effect = mod.MagicEffects.AddNew("LZC_SetUpCampEffect");
        effect.Name = " Set Up Camp";
        effect.Archetype = new MagicEffectArchetype { Type = MagicEffectArchetype.TypeEnum.Script };
        effect.CastType = CastType.FireAndForget;
        effect.TargetType = TargetType.Self;
        effect.Flags = MagicEffect.Flag.NoDuration | MagicEffect.Flag.NoMagnitude | MagicEffect.Flag.NoArea
            | MagicEffect.Flag.NoHitEvent | MagicEffect.Flag.Painless;
        effect.VirtualMachineAdapter = Scripts(Script("LZC_SetUpCampEffect", Link("CampQuest", campQuest)));

        var power = mod.Spells.AddNew("LZC_SetUpCampPower");
        // The leading space sorts the power to the top of the magic menu.
        power.Name = " Set Up Camp";
        power.Type = SpellType.LesserPower;
        power.CastType = CastType.FireAndForget;
        power.TargetType = TargetType.Self;
        if (voice is not null)
        {
            power.EquipmentType.SetTo(voice);
        }
        power.Effects.Add(new Effect
        {
            BaseEffect = effect.ToNullableLink(),
            Data = new EffectData { Magnitude = 0, Area = 0, Duration = 0 },
        });

        // The holding cell: map marker, storage, and clutter sets.
        var holding = HoldingCell();
        var mapMarker = Place(holding, MapMarkerKey, new P3Float(0, 0, 0), new P3Float(0, 0, 0), 1, disabled: true);
        mapMarker.EditorID = "LZC_CampMapMarkerRef";
        mapMarker.MapMarker = new MapMarker
        {
            Flags = MapMarker.Flag.Visible | MapMarker.Flag.CanTravelTo,
            Name = "Your Camp",
            Type = MapMarker.MarkerType.Camp,
        };
        var storage = Place(holding, storageContainer.FormKey, new P3Float(0, -500, 0), new P3Float(0, 0, 0), 1);
        storage.EditorID = "LZC_CampStorageRef";

        var storageMarkers = new List<PlacedObject>();
        var fireSet = ClutterSet(holding, "Fire", 0, clutter.Fire, storageMarkers);
        var smallSet = ClutterSet(holding, "SmallTent", 1, clutter.SmallTent, storageMarkers);
        var largeSet = ClutterSet(holding, "LargeTent", 2, clutter.LargeTent, storageMarkers);
        var ringSet = ClutterSet(holding, "RingBedroll", 3, clutter.RingBedroll, storageMarkers);
        var tubSet = ClutterSet(holding, "Tub", 4, clutter.Tub, storageMarkers);

        // Follower aliases. Their packages (sleep in BedrollN, sit in SeatN, by
        // LZC_CampActivity) are made in the Creation Kit: it couldn't load
        // packages written here (see the README).
        const uint playerAlias = 0;
        const uint firstFollowerAlias = 1;
        const uint firstBedrollAlias = firstFollowerAlias + MaxFollowers;
        const uint firstSeatAlias = firstBedrollAlias + MaxFollowers;

        followersQuest.Aliases.Add(new QuestAlias
        {
            ID = playerAlias,
            Name = "Player",
            ForcedReference = new FormLinkNullable<IPlacedGetter>(PlayerRefKey),
        });
        for (uint i = 0; i < MaxFollowers; i++)
        {
            var follower = new QuestAlias
            {
                ID = firstFollowerAlias + i,
                Name = $"Follower{i + 1}",
                Flags = QuestAlias.Flag.Optional | QuestAlias.Flag.AllowReserved
                    | QuestAlias.Flag.MatchingRefInLoadedArea | QuestAlias.Flag.MatchingRefClosest,
            };
            follower.Conditions.Add(Equals(new GetPlayerTeammateConditionData(), 1));
            var near = new GetDistanceConditionData();
            near.Target.Link.SetTo(PlayerRefKey);
            follower.Conditions.Add(new ConditionFloat { Data = near, CompareOperator = CompareOperator.LessThan, ComparisonValue = 2048 });
            for (uint j = 0; j < i; j++)
            {
                // Don't pick someone an earlier alias already holds.
                follower.Conditions.Add(Equals(new GetIsAliasRefConditionData { ReferenceAliasIndex = (int)(firstFollowerAlias + j) }, 0));
            }
            followersQuest.Aliases.Add(follower);
        }
        for (uint i = 0; i < MaxFollowers; i++)
        {
            followersQuest.Aliases.Add(EmptyAlias(firstBedrollAlias + i, $"Bedroll{i + 1}"));
        }
        for (uint i = 0; i < MaxFollowers; i++)
        {
            followersQuest.Aliases.Add(EmptyAlias(firstSeatAlias + i, $"Seat{i + 1}"));
        }
        followersQuest.NextAliasID = firstSeatAlias + MaxFollowers;

        var followersScript = Script("LZC_CampFollowersScript",
            Link("PlayerRef", PlayerRefKey),
            AliasList("Followers", followersQuest, firstFollowerAlias, MaxFollowers),
            AliasList("Bedrolls", followersQuest, firstBedrollAlias, MaxFollowers),
            AliasList("Seats", followersQuest, firstSeatAlias, MaxFollowers),
            Link("LZC_CampActivity", activity));
        if (partnerFaction is not null)
        {
            followersScript.Properties.Add(Link("PartnerFaction", partnerFaction.FormKey));
        }
        followersQuest.VirtualMachineAdapter = new QuestAdapter
        {
            Version = ScriptVersion,
            ObjectFormat = ScriptObjectFormat,
            Scripts = { followersScript },
            Aliases = { AliasScript(followersQuest, playerAlias, "LZC_CampPlayerAliasScript") },
        };

        // The controller quest.
        campQuest.Flags = Quest.Flag.StartGameEnabled | Quest.Flag.RunOnce;
        var campScript = Script("LZC_CampsiteQuestScript",
            Link("PlayerRef", PlayerRefKey),
            Link("LZC_SetUpCampPower", power),
            Links("LZC_SmallTentKits", smallKits),
            Links("LZC_LargeTentKits", largeKits),
            Link("LZC_BedrollKit", bedrollKit),
            Links("LZC_SmallTents", smallTents),
            Links("LZC_LargeTents", largeTents),
            Link("LZC_SitMarkerPlayer", playerSeat),
            Link("LZC_BedrollSitMarker", bedrollSeat),
            Link("LZC_SitMarkerFollower", sitMarker.FormKey),
            Link("LZC_FireClutterAnchor", fireSet.Anchor),
            Links("LZC_FireClutter", fireSet.Items),
            Link("LZC_SmallTentClutterAnchor", smallSet.Anchor),
            Links("LZC_SmallTentClutter", smallSet.Items),
            Link("LZC_LargeTentClutterAnchor", largeSet.Anchor),
            Links("LZC_LargeTentClutter", largeSet.Items),
            Link("LZC_RingBedrollClutterAnchor", ringSet.Anchor),
            Links("LZC_RingBedrollClutter", ringSet.Items),
            Link("LZC_TubClutterAnchor", tubSet.Anchor),
            Links("LZC_TubClutter", tubSet.Items),
            Link("LZC_CampStorage", storage),
            Links("LZC_CampStorageMarkers", storageMarkers),
            Link("LZC_PackUpConfirmMsg", packUpMsg),
            Link("LZC_CampMapMarker", mapMarker),
            Link("LZC_CampFollowersQuest", followersQuest));
        AddIfFound(campScript, "LZC_BedrollPlaced", bedroll);
        AddIfFound(campScript, "LZC_CampfirePlaced", campfire);
        AddIfFound(campScript, "LZC_CookingStation", cookingStation);
        AddIfFound(campScript, "LZC_Tub", tub);
        AddIfFound(campScript, "LZC_CampfireLight", campfireLight);
        AddIfFound(campScript, "LZC_Firepit", firepit);
        AddIfFound(campScript, "LZC_ChoppingBlock", choppingBlock);
        AddIfFound(campScript, "LZC_UnpackSound", unpackSound);
        AddIfFound(campScript, "Firewood01", firewood);
        AddIfFound(campScript, "LocTypeCity", locTypeCity);
        AddIfFound(campScript, "LocTypeTown", locTypeTown);

        campQuest.VirtualMachineAdapter = new QuestAdapter
        {
            Version = ScriptVersion,
            ObjectFormat = ScriptObjectFormat,
            Scripts = { campScript },
        };
        return mod;
    }

    // ---------------------------------------------------------------------
    // Lookups

    T? Find<T>(string editorId, string hint) where T : class, ISkyrimMajorRecordGetter
    {
        if (string.IsNullOrWhiteSpace(editorId))
        {
            return null;
        }
        var hit = skyrim.EnumerateMajorRecords<T>()
            .FirstOrDefault(r => string.Equals(r.EditorID, editorId, StringComparison.OrdinalIgnoreCase));
        if (hit is null)
        {
            // Suggest matches of the right type first, then of any type.
            var near = skyrim.EnumerateMajorRecords<T>()
                .Select(r => r.EditorID)
                .Where(id => id is not null && id.Contains(hint, StringComparison.OrdinalIgnoreCase))
                .Take(15)
                .ToList();
            var other = near.Count > 0 ? [] : Search(skyrim, hint).Take(15).ToList();
            var kind = typeof(T).Name.TrimStart('I').Replace("Getter", "");
            problems.Add($"No {kind} \"{editorId}\" in Skyrim.esm." +
                (near.Count > 0 ? $" Close matches: {string.Join(", ", near)}." : "") +
                (other.Count > 0 ? $" No {kind} contains \"{hint}\", but these other records do: {string.Join(", ", other)}." : "") +
                $" Run with --find {hint} to list them all.");
        }
        return hit;
    }

    /// <summary>Every record whose EditorID contains the text, as "EditorID (Type)".</summary>
    public static IEnumerable<string> Search(ISkyrimModGetter mod, string text) =>
        mod.EnumerateMajorRecords()
            .Where(r => r.EditorID is not null && r.EditorID.Contains(text, StringComparison.OrdinalIgnoreCase))
            .Select(r => $"{r.EditorID} ({r.GetType().Name.Replace("BinaryOverlay", "")}, {r.FormKey})")
            .OrderBy(s => s, StringComparer.OrdinalIgnoreCase);

    // ---------------------------------------------------------------------
    // Records

    MiscItem MiscItem(string editorId, KitInfo info, string colour, string model)
    {
        var item = mod.MiscItems.AddNew(editorId);
        item.Name = info.Name.Replace("{Colour}", colour);
        item.Weight = info.Weight;
        item.Value = info.Value;
        item.Model = new Model { File = model };
        item.ObjectBounds = Bounds(20, 20, 15);
        return item;
    }

    Activator Tent(string size, string colour, string model, string shape, short halfWidth, short halfDepth, Quest campQuest)
    {
        var tent = mod.Activators.AddNew($"LZC_Tent{size}{colour}");
        tent.Name = "Tent";
        tent.Model = new Model { File = model };
        tent.ObjectBounds = Bounds(halfWidth, halfDepth, 180);
        tent.VirtualMachineAdapter = Scripts(Script("LZC_TentActivatorScript", Link("CampQuest", campQuest)));

        // Brown is the mesh's own texture; other colours swap the canvas.
        if (colour != "Brown" && !string.IsNullOrWhiteSpace(shape))
        {
            var textures = mod.TextureSets.AddNew($"LZC_TentTexture{size}{colour}");
            textures.Diffuse = settings.TentTextures.Path.Replace("{Size}", size)
                .Replace("{Colour}", settings.TentTextures.FileColours.GetValueOrDefault(colour, colour));
            tent.Model.AlternateTextures = [new AlternateTexture { Name = shape, NewTexture = textures.ToLink(), Index = 0 }];
        }
        return tent;
    }

    // Papyrus plays sound markers: a vanilla one is used as it is, and a
    // vanilla sound descriptor gets one of our own, editorId.
    IMajorRecordGetter? Sound(string editorId, string vanilla, string hint)
    {
        if (string.IsNullOrWhiteSpace(vanilla))
        {
            return null;
        }
        var vanillaMarker = skyrim.EnumerateMajorRecords<ISoundMarkerGetter>()
            .FirstOrDefault(r => string.Equals(r.EditorID, vanilla, StringComparison.OrdinalIgnoreCase));
        if (vanillaMarker is not null)
        {
            return vanillaMarker;
        }
        var descriptor = Find<ISoundDescriptorGetter>(vanilla, hint);
        if (descriptor is null)
        {
            return null;
        }
        var marker = mod.SoundMarkers.AddNew(editorId);
        marker.ObjectBounds = Bounds(0, 0, 0);
        marker.SoundDescriptor.SetTo(descriptor);
        return marker;
    }

    Furniture Seat(string editorId, IFurnitureGetter template, string model)
    {
        var seat = mod.Furniture.DuplicateInAsNewRecord(template, editorId);
        seat.Name = "Seat";
        seat.Model = new Model { File = model };
        return seat;
    }

    Cell HoldingCell()
    {
        var cell = new Cell(mod.GetNextFormKey(), SkyrimRelease.SkyrimSE)
        {
            EditorID = "LZC_HoldingCell",
            Name = "Lazy Camping Holding Cell",
            Flags = Cell.Flag.IsInteriorCell,
        };
        // Interior cells are grouped into blocks by the last two digits of their FormID.
        var id = (int)cell.FormKey.ID;
        mod.Cells.Records.Add(new CellBlock
        {
            BlockNumber = id % 10,
            GroupType = GroupTypeEnum.InteriorCellBlock,
            SubBlocks =
            {
                new CellSubBlock
                {
                    BlockNumber = id / 10 % 10,
                    GroupType = GroupTypeEnum.InteriorCellSubBlock,
                    Cells = { cell },
                },
            },
        });
        return cell;
    }

    PlacedObject Place(Cell cell, FormKey baseObject, P3Float position, P3Float rotationDegrees, float scale, bool disabled = false)
    {
        var placed = new PlacedObject(mod.GetNextFormKey(), SkyrimRelease.SkyrimSE)
        {
            Base = new FormLinkNullable<IPlaceableObjectGetter>(baseObject),
            Placement = new Placement
            {
                Position = position,
                Rotation = new P3Float(Radians(rotationDegrees.X), Radians(rotationDegrees.Y), Radians(rotationDegrees.Z)),
            },
        };
        placed.MajorRecordFlagsRaw |= PersistentFlag;
        if (disabled)
        {
            placed.SkyrimMajorRecordFlags |= SkyrimMajorRecord.SkyrimMajorRecordFlag.InitiallyDisabled;
        }
        if (Math.Abs(scale - 1) > 0.001f)
        {
            placed.Scale = scale;
        }
        cell.Persistent.Add(placed);
        return placed;
    }

    (PlacedObject Anchor, List<PlacedObject> Items) ClutterSet(Cell cell, string name, int slot, List<ClutterItem> items, List<PlacedObject> storageMarkers)
    {
        // Sets sit 1000 units apart in the holding cell so they're easy to tell apart in the Creation Kit.
        var origin = new P3Float(slot * 1000, 1000, 0);
        var anchor = Place(cell, XMarkerHeadingKey, origin, new P3Float(0, 0, 0), 1);
        anchor.EditorID = $"LZC_{name}ClutterAnchor";

        var placed = new List<PlacedObject>();
        foreach (var item in items)
        {
            var baseKey = ResolveBase(item.Base);
            if (baseKey is null)
            {
                continue;
            }
            var offset = item.Offset.Length >= 3 ? item.Offset : [0, 0, 0];
            var rotation = item.Rotation.Length >= 3 ? item.Rotation : [0, 0, 0];
            var reference = Place(cell, baseKey.Value,
                new P3Float(origin.X + offset[0], origin.Y + offset[1], origin.Z + offset[2]),
                new P3Float(rotation[0], rotation[1], rotation[2]),
                item.Scale);
            placed.Add(reference);
            if (item.Storage)
            {
                reference.EditorID = $"LZC_{name}StorageMarker";
                storageMarkers.Add(reference);
            }
        }
        return (anchor, placed);
    }

    FormKey? ResolveBase(string editorId)
    {
        if (string.Equals(editorId, "XMarkerHeading", StringComparison.OrdinalIgnoreCase))
        {
            return XMarkerHeadingKey;
        }
        var ours = mod.EnumerateMajorRecords<IPlaceableObjectGetter>()
            .FirstOrDefault(r => string.Equals(r.EditorID, editorId, StringComparison.OrdinalIgnoreCase));
        if (ours is not null)
        {
            return ours.FormKey;
        }
        return Find<IPlaceableObjectGetter>(editorId, editorId.Length > 4 ? editorId[..4] : editorId)?.FormKey;
    }

    // ---------------------------------------------------------------------
    // Small helpers

    static float Radians(float degrees) => degrees * MathF.PI / 180f;

    static ObjectBounds Bounds(short halfX, short halfY, short height) => new()
    {
        First = new P3Int16((short)-halfX, (short)-halfY, 0),
        Second = new P3Int16(halfX, halfY, height),
    };

    static ConditionFloat Equals(ConditionData data, float value) =>
        new() { Data = data, CompareOperator = CompareOperator.EqualTo, ComparisonValue = value };

    static QuestAlias EmptyAlias(uint id, string name) => new()
    {
        ID = id,
        Name = name,
        Flags = QuestAlias.Flag.Optional,
    };

    static VirtualMachineAdapter Scripts(params ScriptEntry[] scripts)
    {
        var adapter = new VirtualMachineAdapter { Version = ScriptVersion, ObjectFormat = ScriptObjectFormat };
        adapter.Scripts.AddRange(scripts);
        return adapter;
    }

    static ScriptEntry Script(string name, params ScriptProperty[] properties)
    {
        var script = new ScriptEntry { Name = name };
        script.Properties.AddRange(properties);
        return script;
    }

    static ScriptObjectProperty Link(string name, IMajorRecordGetter record) => Link(name, record.FormKey);

    static ScriptObjectProperty Link(string name, FormKey key) => new()
    {
        Name = name,
        Flags = ScriptProperty.Flag.Edited,
        Object = new FormLink<ISkyrimMajorRecordGetter>(key),
    };

    static ScriptObjectListProperty Links(string name, IEnumerable<IMajorRecordGetter> records)
    {
        var list = new ScriptObjectListProperty { Name = name, Flags = ScriptProperty.Flag.Edited };
        foreach (var record in records)
        {
            list.Objects.Add(new ScriptObjectProperty { Object = new FormLink<ISkyrimMajorRecordGetter>(record.FormKey) });
        }
        return list;
    }

    static ScriptObjectListProperty AliasList(string name, Quest quest, uint firstAlias, int count)
    {
        var list = new ScriptObjectListProperty { Name = name, Flags = ScriptProperty.Flag.Edited };
        for (var i = 0; i < count; i++)
        {
            list.Objects.Add(new ScriptObjectProperty
            {
                Object = new FormLink<ISkyrimMajorRecordGetter>(quest.FormKey),
                Alias = (short)(firstAlias + i),
            });
        }
        return list;
    }

    static QuestFragmentAlias AliasScript(Quest quest, uint alias, string script) => new()
    {
        Property = new ScriptObjectProperty
        {
            Object = new FormLink<ISkyrimMajorRecordGetter>(quest.FormKey),
            Alias = (short)alias,
        },
        Version = ScriptVersion,
        ObjectFormat = ScriptObjectFormat,
        Scripts = { new ScriptEntry { Name = script } },
    };

    static void AddIfFound(ScriptEntry script, string property, IMajorRecordGetter? record)
    {
        if (record is not null)
        {
            script.Properties.Add(Link(property, record));
        }
    }
}
