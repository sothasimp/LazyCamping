#include "Serialization.h"

#include "FormNames.h"
#include "GrassHider.h"
#include "Shelter.h"

namespace Serialization
{
	namespace
	{
		constexpr std::uint32_t kRecordId = 'LZCG';

		void OnSave(SKSE::SerializationInterface* a_intfc)
		{
			GrassHider::Save(a_intfc);
			FormNames::Save(a_intfc);
			Shelter::Save(a_intfc);
		}

		void OnRevert(SKSE::SerializationInterface*)
		{
			GrassHider::Revert();
			FormNames::Revert();
			Shelter::Revert();
		}

		void OnLoad(SKSE::SerializationInterface* a_intfc)
		{
			std::uint32_t type = 0;
			std::uint32_t version = 0;
			std::uint32_t length = 0;
			while (a_intfc->GetNextRecordInfo(type, version, length)) {
				if (!GrassHider::Load(a_intfc, type, version) && !FormNames::Load(a_intfc, type, version) &&
					!Shelter::Load(a_intfc, type, version)) {
					logger::warn("Unknown co-save record {:08X}", type);
				}
			}
		}
	}

	void Install()
	{
		auto* serialization = SKSE::GetSerializationInterface();
		serialization->SetUniqueID(kRecordId);
		serialization->SetSaveCallback(OnSave);
		serialization->SetRevertCallback(OnRevert);
		serialization->SetLoadCallback(OnLoad);
	}
}
