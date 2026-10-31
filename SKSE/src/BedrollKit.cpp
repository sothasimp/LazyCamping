#include "BedrollKit.h"

#include "ScriptObjects.h"

namespace BedrollKit
{
	namespace
	{
		constexpr auto          kShape = "cloth"sv;
		constexpr std::uint32_t kShapeIndex = 5;  // the cloth shape's index in the kit's mesh

		using AlternateTexture = RE::TESModelTextureSwap::AlternateTexture;

		AlternateTexture* swap{ nullptr };      // ours, allocated once
		AlternateTexture* original{ nullptr };  // the kit's own, from the plugin
		std::uint32_t     originalCount{ 0 };
		bool              saved{ false };

		template <class T>
		T* Property(RE::BSScript::Object* a_script, std::string_view a_name)
		{
			auto* form = a_script ? ScriptObjects::FormValue(ScriptObjects::AutoProperty(a_script, a_name)) : nullptr;
			return form ? form->As<T>() : nullptr;
		}
	}

	void Apply(RE::BSScript::Object* a_script, std::int32_t a_style)
	{
		auto* kit = Property<RE::TESObjectMISC>(a_script, "LZC_BedrollKit");
		if (!kit) {
			return;
		}
		if (!saved) {
			original = kit->alternateTextures;
			originalCount = kit->numAlternateTextures;
			saved = true;
		}

		const char* set = nullptr;
		switch (a_style) {
		case 1:
		case 4:
			set = "LZC_BedrollTextureGreen";
			break;
		case 2:
			set = "LZC_BedrollTextureBlue";
			break;
		case 3:
			set = "LZC_BedrollTextureRed";
			break;
		default:
			break;
		}
		auto* textureSet = set ? Property<RE::BGSTextureSet>(a_script, set) : nullptr;
		if (!textureSet) {
			kit->alternateTextures = original;
			kit->numAlternateTextures = originalCount;
			return;
		}

		if (!swap) {
			// From the game's heap, like the plugin's own alternate textures.
			swap = RE::calloc<AlternateTexture>(1);
		}
		swap->textureSet = textureSet;
		swap->index3D = kShapeIndex;
		swap->name3D = kShape;
		kit->alternateTextures = swap;
		kit->numAlternateTextures = 1;
	}
}
