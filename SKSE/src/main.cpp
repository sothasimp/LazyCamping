#include "BedrollSeats.h"
#include "GrassHider.h"
#include "Menu.h"
#include "Papyrus.h"
#include "Serialization.h"

// SKSEPluginInfo (name, version, Address Library compatibility) is generated
// by the commonlibsse-ng.plugin xmake rule.

namespace
{
	void InitLogging()
	{
		auto path = logger::log_directory();
		if (!path) {
			return;
		}
		*path /= "LazyCamping.log";

		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
		auto log = std::make_shared<spdlog::logger>("global", std::move(sink));
		log->set_level(spdlog::level::info);
		log->flush_on(spdlog::level::info);
		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("[%H:%M:%S] [%l] %v");
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);
	InitLogging();

	if (!SKSE::GetPapyrusInterface()->Register(Papyrus::Register)) {
		logger::critical("Failed to register Papyrus functions");
		return false;
	}

	Serialization::Install();
	GrassHider::Install();
	BedrollSeats::Install();

	// SKSE Menu Framework loads after this plugin, if it's installed at all.
	SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* a_message) {
		if (a_message->type == SKSE::MessagingInterface::kPostLoad) {
			Menu::Register();
		} else if (a_message->type == SKSE::MessagingInterface::kPostLoadGame ||
				   a_message->type == SKSE::MessagingInterface::kNewGame) {
			Menu::ApplyLoadedSettings();
		}
	});

	logger::info("LazyCamping loaded");
	return true;
}
