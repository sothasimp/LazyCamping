#include "Papyrus.h"

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

	logger::info("LazyCamping loaded");
	return true;
}
