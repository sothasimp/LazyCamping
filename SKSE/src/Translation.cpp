#include "Translation.h"

namespace Translation
{
	namespace
	{
		constexpr auto kFilePrefix = "Data/Interface/Translations/Wayfarer - A Convenient Camping Framework_"sv;

		std::unordered_map<std::string, std::string> strings;
		std::once_flag                               loadOnce;

		std::string Language()
		{
			auto* settings = RE::INISettingCollection::GetSingleton();
			auto* setting = settings ? settings->GetSetting("sLanguage:General") : nullptr;
			std::string language = setting && setting->GetType() == RE::Setting::Type::kString && setting->GetString() ? setting->GetString() : "";
			if (language.empty()) {
				language = "ENGLISH";
			}
			std::ranges::transform(language, language.begin(), [](unsigned char a_c) { return static_cast<char>(std::toupper(a_c)); });
			return language;
		}

		void AppendUtf8(std::string& a_out, char32_t a_c)
		{
			if (a_c < 0x80) {
				a_out += static_cast<char>(a_c);
			} else if (a_c < 0x800) {
				a_out += static_cast<char>(0xC0 | (a_c >> 6));
				a_out += static_cast<char>(0x80 | (a_c & 0x3F));
			} else if (a_c < 0x10000) {
				a_out += static_cast<char>(0xE0 | (a_c >> 12));
				a_out += static_cast<char>(0x80 | ((a_c >> 6) & 0x3F));
				a_out += static_cast<char>(0x80 | (a_c & 0x3F));
			} else {
				a_out += static_cast<char>(0xF0 | (a_c >> 18));
				a_out += static_cast<char>(0x80 | ((a_c >> 12) & 0x3F));
				a_out += static_cast<char>(0x80 | ((a_c >> 6) & 0x3F));
				a_out += static_cast<char>(0x80 | (a_c & 0x3F));
			}
		}

		// The file as UTF-8 (ImGui's encoding).
		std::string ReadUtf8(const std::filesystem::path& a_path)
		{
			std::ifstream file(a_path, std::ios::binary);
			if (!file) {
				return {};
			}
			const std::string bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			if (bytes.size() >= 2 && static_cast<unsigned char>(bytes[0]) == 0xFF && static_cast<unsigned char>(bytes[1]) == 0xFE) {
				std::string text;
				for (std::size_t i = 2; i + 1 < bytes.size(); i += 2) {
					char32_t unit = static_cast<unsigned char>(bytes[i]) | static_cast<unsigned char>(bytes[i + 1]) << 8;
					if (unit >= 0xD800 && unit < 0xDC00 && i + 3 < bytes.size()) {
						const char32_t low = static_cast<unsigned char>(bytes[i + 2]) | static_cast<unsigned char>(bytes[i + 3]) << 8;
						if (low >= 0xDC00 && low < 0xE000) {
							unit = 0x10000 + ((unit - 0xD800) << 10) + (low - 0xDC00);
							i += 2;
						}
					}
					AppendUtf8(text, unit);
				}
				return text;
			}
			if (bytes.starts_with("\xEF\xBB\xBF")) {
				return bytes.substr(3);
			}
			return bytes;
		}

		bool LoadFile(const std::string& a_language)
		{
			const std::filesystem::path path{ std::string(kFilePrefix) + a_language + ".txt" };
			std::error_code error;
			if (!std::filesystem::is_regular_file(path, error)) {
				return false;
			}
			std::istringstream text(ReadUtf8(path));
			std::size_t        count = 0;
			for (std::string line; std::getline(text, line);) {
				if (!line.empty() && line.back() == '\r') {
					line.pop_back();
				}
				const auto tab = line.find('\t');
				if (!line.starts_with('$') || tab == std::string::npos) {
					continue;
				}
				strings[line.substr(0, tab)] = line.substr(tab + 1);
				++count;
			}
			logger::info("Read {} menu string(s) from {}", count, path.string());
			return true;
		}

		void Load()
		{
			const auto language = Language();
			if (!LoadFile(language) && language != "ENGLISH") {
				logger::info("No menu translation for {}; using English", language);
			}
		}
	}

	const char* T(std::string_view a_key, const char* a_english)
	{
		std::call_once(loadOnce, Load);
		const auto it = strings.find(std::string(a_key));
		return it != strings.end() && !it->second.empty() ? it->second.c_str() : a_english;
	}
}
