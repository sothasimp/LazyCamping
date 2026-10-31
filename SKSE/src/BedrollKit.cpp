#include "BedrollKit.h"

#include "ScriptObjects.h"

namespace BedrollKit
{
	namespace
	{
		constexpr auto          kShape = "cloth"sv;
		constexpr std::uint32_t kShapeIndex = 0;  // the cloth shape's index in the kit's mesh, as the Creation Kit counts

		using AlternateTexture = RE::TESModelTextureSwap::AlternateTexture;

		// A form whose "cloth" shape follows the style: the kit, and the
		// rolled-up bedroll dropped at camp.
		struct Swappable
		{
			AlternateTexture* swap{ nullptr };      // ours, allocated once
			AlternateTexture* original{ nullptr };  // the form's own, from the plugin
			std::uint32_t     originalCount{ 0 };
			bool              saved{ false };

			void Apply(RE::TESModelTextureSwap* a_model, RE::BGSTextureSet* a_textureSet)
			{
				if (!a_model) {
					return;
				}
				if (!saved) {
					original = a_model->alternateTextures;
					originalCount = a_model->numAlternateTextures;
					saved = true;
				}
				if (!a_textureSet) {
					a_model->alternateTextures = original;
					a_model->numAlternateTextures = originalCount;
					return;
				}
				if (!swap) {
					// From the game's heap, like the plugin's own alternate textures.
					swap = RE::calloc<AlternateTexture>(1);
				}
				swap->textureSet = a_textureSet;
				swap->index3D = kShapeIndex;
				swap->name3D = kShape;
				a_model->alternateTextures = swap;
				a_model->numAlternateTextures = 1;
			}
		};

		Swappable kit;
		Swappable rolled;

		template <class T>
		T* Property(RE::BSScript::Object* a_script, std::string_view a_name)
		{
			auto* form = a_script ? ScriptObjects::FormValue(ScriptObjects::AutoProperty(a_script, a_name)) : nullptr;
			return form ? form->As<T>() : nullptr;
		}
	}

	void Apply(RE::BSScript::Object* a_script, std::int32_t a_style)
	{
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
		kit.Apply(Property<RE::TESObjectMISC>(a_script, "LZC_BedrollKit"), textureSet);
		rolled.Apply(Property<RE::TESObjectACTI>(a_script, "LZC_BedrollRolled"), textureSet);
	}
}
