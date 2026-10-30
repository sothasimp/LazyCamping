#include "BedrollKit.h"

namespace BedrollKit
{
	namespace
	{
		constexpr auto       kPlugin = "LazyCamping.esp"sv;
		constexpr auto       kShape = "cloth"sv;
		constexpr std::uint32_t kShapeIndex = 5;  // the cloth shape's index in the kit's mesh
		constexpr RE::FormID kKit = 0x803;     // LZC_BedrollKit
		constexpr RE::FormID kGreen = 0x5403;  // LZC_BedrollTextureGreen
		constexpr RE::FormID kBlue = 0x5404;   // LZC_BedrollTextureBlue
		constexpr RE::FormID kRed = 0x5405;    // LZC_BedrollTextureRed

		using AlternateTexture = RE::TESModelTextureSwap::AlternateTexture;

		AlternateTexture* swap{ nullptr };      // ours, allocated once
		AlternateTexture* original{ nullptr };  // the kit's own, from the plugin
		std::uint32_t     originalCount{ 0 };
		bool              saved{ false };
	}

	void Apply(std::int32_t a_style)
	{
		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		auto* kit = dataHandler ? dataHandler->LookupForm<RE::TESObjectMISC>(kKit, kPlugin) : nullptr;
		if (!kit) {
			return;
		}
		if (!saved) {
			original = kit->alternateTextures;
			originalCount = kit->numAlternateTextures;
			saved = true;
		}

		RE::FormID set = 0;
		switch (a_style) {
		case 1:
		case 4:
			set = kGreen;
			break;
		case 2:
			set = kBlue;
			break;
		case 3:
			set = kRed;
			break;
		default:
			break;
		}
		auto* textureSet = set ? dataHandler->LookupForm<RE::BGSTextureSet>(set, kPlugin) : nullptr;
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
