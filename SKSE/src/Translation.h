#pragma once

// The SKSE menu's text in the game's language, from
// Data/Interface/Translations/Wayfarer - A Convenient Camping Framework_<LANGUAGE>.txt:
// the usual Skyrim translation file (UTF-16 LE, or UTF-8), one "$KEY<Tab>text"
// a line. SKSE loads the same file for the game's own interface, so $KEY
// strings work there too. Keys missing from it (or with no file for the
// language) show in English.
namespace Translation
{
	// The text for a_key ("$LZC_..."), or a_english. The pointer stays valid.
	const char* T(std::string_view a_key, const char* a_english);
}
