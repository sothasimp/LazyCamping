#pragma once

// Reaching Papyrus scripts from C++. Main thread only.
namespace ScriptObjects
{
	// The script a_scriptName bound to a quest from a plugin whose name starts
	// with a_pluginPrefix, if a game is loaded. a_cache remembers the quest.
	RE::BSTSmartPointer<RE::BSScript::Object> FindQuestScript(std::string_view a_pluginPrefix, const char* a_scriptName, RE::FormID& a_cache);

	// The value of an Auto property, which lives in a hidden variable.
	RE::BSScript::Variable* AutoProperty(RE::BSScript::Object* a_object, std::string_view a_name);

	// The form an object value (or property) holds, or nullptr.
	RE::TESForm* FormValue(const RE::BSScript::Variable* a_value);

	// Calls a function on a script, queued like any Papyrus call.
	template <class... Args>
	void Call(const RE::BSTSmartPointer<RE::BSScript::Object>& a_object, std::string_view a_function, Args&&... a_args)
	{
		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		if (!vm || !a_object) {
			return;
		}
		auto                                                    object = a_object;
		RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
		vm->DispatchMethodCall(object, RE::BSFixedString(a_function), RE::MakeFunctionArguments(std::forward<Args>(a_args)...), callback);
	}
}
