#pragma once

#include <charconv>
#include <cmath>
#include <optional>
#include <string_view>
#include <type_traits>

namespace RE { class TESQuest; }

namespace Settings
{
	// Called only by the campsite quest's OnInit, never when loading its saved settings.
	void ApplyDefaults(RE::TESQuest* a_quest);

	// The menu orders medium before bright; the saved property predates medium.
	constexpr int StoredLightLevel(int a_slider) { return a_slider == 1 ? 1 : a_slider == 2 ? 3 : 2; }

	namespace detail
	{
		// Require the entire value to parse so typos cannot silently select another setting.
		template <class T>
		std::optional<T> ParseValue(std::string_view a_text, T a_min, T a_max)
		{
			const auto first = a_text.find_first_not_of(" \t\r\n");
			if (first == std::string_view::npos) {
				return std::nullopt;
			}
			a_text = a_text.substr(first, a_text.find_last_not_of(" \t\r\n") - first + 1);
			T value{};
			if constexpr (std::is_same_v<T, bool>) {
				const auto equal = [&](std::string_view a_word) {
					if (a_text.size() != a_word.size()) {
						return false;
					}
					for (std::size_t i = 0; i < a_text.size(); ++i) {
						const char c = a_text[i] >= 'A' && a_text[i] <= 'Z' ? a_text[i] + ('a' - 'A') : a_text[i];
						if (c != a_word[i]) {
							return false;
						}
					}
					return true;
				};
				if (a_text == "1" || equal("true")) {
					value = true;
				} else if (a_text == "0" || equal("false")) {
					value = false;
				} else {
					return std::nullopt;
				}
			} else {
				const auto result = std::from_chars(a_text.data(), a_text.data() + a_text.size(), value);
				if (result.ec != std::errc{} || result.ptr != a_text.data() + a_text.size()) {
					return std::nullopt;
				}
				if constexpr (std::is_floating_point_v<T>) {
					if (!std::isfinite(value)) {
						return std::nullopt;
					}
				}
			}
			return value >= a_min && value <= a_max ? std::optional<T>{ value } : std::nullopt;
		}
	}
}
