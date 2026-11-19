#include "../src/Settings.h"

#include <iostream>
#include <cstdlib>

namespace
{
	void Check(bool a_condition, const char* a_message)
	{
		if (!a_condition) {
			std::cerr << a_message << '\n';
			std::exit(EXIT_FAILURE);
		}
	}
}

int main()
{
	using Settings::detail::ParseValue;

	// Boolean typos must not silently turn a feature on or off.
	Check(ParseValue<bool>("TrUe", false, true) == true, "case-insensitive true");
	Check(ParseValue<bool>(" FALSE \t", false, true) == false, "trimmed false");
	Check(ParseValue<bool>("1", false, true) == true, "numeric true");
	Check(ParseValue<bool>("0", false, true) == false, "numeric false");
	for (auto text : { "", "  ", "tru", "falsex", "2", "-1", "yes", "true ; comment" }) {
		Check(!ParseValue<bool>(text, false, true), "reject invalid boolean");
	}

	// Reject partial numbers, overflow and choices outside the menu's range.
	Check(ParseValue<int>(" 6 ", 1, 6) == 6, "upper firewood boundary");
	Check(ParseValue<int>("1", 1, 6) == 1, "lower firewood boundary");
	Check(ParseValue<int>("0", 0, 8) == 0, "vanilla style");
	Check(ParseValue<int>("8", 0, 8) == 8, "last bedroll style");
	for (auto text : { "", "0", "7", "-1", "2.5", "3junk", "2147483648", "0x2" }) {
		Check(!ParseValue<int>(text, 1, 6), "reject invalid integer");
	}

	// Non-finite values would bypass ordinary comparisons unless explicitly rejected.
	Check(ParseValue<float>("1.0", 1.0f, 24.0f) == 1.0f, "lower burn time boundary");
	Check(ParseValue<float>("24", 1.0f, 24.0f) == 24.0f, "upper burn time boundary");
	Check(ParseValue<float>("6.5", 1.0f, 24.0f) == 6.5f, "fractional burn time");
	for (auto text : { "", "0.9", "24.1", "nan", "inf", "-inf", "1e999", "6hours" }) {
		Check(!ParseValue<float>(text, 1.0f, 24.0f), "reject invalid float");
	}

	Check(Settings::StoredLightLevel(1) == 1, "dim light mapping");
	Check(Settings::StoredLightLevel(2) == 3, "medium light mapping");
	Check(Settings::StoredLightLevel(3) == 2, "bright light mapping");
	std::cout << "Settings value tests passed\n";
}
