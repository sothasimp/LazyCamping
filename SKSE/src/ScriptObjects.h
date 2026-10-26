#pragma once

// Reaching Papyrus scripts from C++. Main thread only.
namespace ScriptObjects
{
	// The script a_scriptName bound to a quest from a plugin whose name starts
	// with a_pluginPrefix, if a game is loaded. a_cache remembers the quest.
	RE::BSTSmartPointer<RE::BSScript::Object> FindQuestScript(std::string_view a_pluginPrefix, const char* a_scriptName, RE::FormID& a_cache);

	// The value of an Auto property, which lives in a hidden variable.
	RE::BSScript::Variable* AutoProperty(RE::BSScript::Object* a_object, std::string_view a_name);
}
