#include "ScriptObjects.h"

namespace ScriptObjects
{
	RE::BSTSmartPointer<RE::BSScript::Object> FindQuestScript(std::string_view a_pluginPrefix, const char* a_scriptName, RE::FormID& a_cache)
	{
		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		auto* policy = vm ? vm->GetObjectHandlePolicy() : nullptr;
		if (!policy) {
			return {};
		}
		const auto bound = [&](RE::TESQuest* a_quest) {
			RE::BSTSmartPointer<RE::BSScript::Object> object;
			const auto handle = policy->GetHandleForObject(a_quest->GetFormType(), a_quest);
			if (handle != policy->EmptyHandle()) {
				vm->FindBoundObject(handle, a_scriptName, object);
			}
			return object;
		};

		if (auto* quest = a_cache ? RE::TESForm::LookupByID<RE::TESQuest>(a_cache) : nullptr) {
			return bound(quest);
		}
		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler) {
			return {};
		}
		for (auto* quest : dataHandler->GetFormArray<RE::TESQuest>()) {
			const auto* file = quest ? quest->GetFile(0) : nullptr;
			if (!file || !file->GetFilename().starts_with(a_pluginPrefix)) {
				continue;
			}
			if (auto object = bound(quest)) {
				a_cache = quest->GetFormID();
				return object;
			}
		}
		return {};
	}

	RE::BSScript::Variable* AutoProperty(RE::BSScript::Object* a_object, std::string_view a_name)
	{
		// An Auto property's value is in a hidden variable named ::Name_var.
		return a_object->GetVariable("::" + std::string(a_name) + "_var");
	}

	RE::TESForm* FormValue(const RE::BSScript::Variable* a_value)
	{
		if (!a_value || !a_value->IsObject()) {
			return nullptr;
		}
		const auto object = a_value->GetObject();
		// A form's handle carries its FormID in the low 32 bits.
		return object ? RE::TESForm::LookupByID(static_cast<RE::FormID>(object->GetHandle() & 0xFFFFFFFF)) : nullptr;
	}
}
