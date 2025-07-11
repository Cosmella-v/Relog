#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include "Console.hpp"

using namespace geode::prelude;

auto convertTime(auto timePoint) {
    auto timeEpoch = std::chrono::system_clock::to_time_t(timePoint);
    return fmt::localtime(timeEpoch);
}

Severity fromString(std::string severity) {
    if (severity == "debug") return Severity::Debug;
    if (severity == "info") return Severity::Info;
    if (severity == "warning") return Severity::Warning;
    if (severity == "error") return Severity::Error;
    return Severity::Info;
}

void vlogImpl_H(Severity severity, Mod* mod, fmt::string_view format, fmt::format_args args) {
	log::vlogImpl(severity, mod, format, args);

    if (!mod->isLoggingEnabled()) return;
    if (severity < mod->getLogLevel()) return;

    Mod* geodeMod = Loader::get()->getLoadedMod("geode.loader");
    std::string level = geodeMod->getSettingValue<std::string>("console-log-level");
    if (severity < fromString(level)) return;

    Log log {
        mod,
        severity,
        fmt::vformat(format, args),
        thread::getName(),
        convertTime(std::chrono::system_clock::now())
    };

    queueInMainThread([log] {
        if (auto console = Console::get()) {
            console->pushLog(log);
        }
    });
}

$on_mod(Loaded) {
    (void) Mod::get()->hook(
        reinterpret_cast<void*>(addresser::getNonVirtual(&log::vlogImpl)),
        &vlogImpl_H,
        "log::vlogImpl"
    );
    queueInMainThread([] {
        SceneManager::get()->keepAcrossScenes(Console::create());
    });
}

class $modify(MenuLayer) {

    bool init() {
        if (!MenuLayer::init()) return false;


        return true;
    }

};