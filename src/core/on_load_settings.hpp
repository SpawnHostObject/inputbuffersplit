//@ core/on_load_settings.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
// v2.99.1: startup checkpoints in the Geode log (each written once), to find where a phone
// that crashes on start stops
#define CBF_STEP(name)                                  \
    do {                                                \
        static bool cbfStepDone = false;                \
        if (!cbfStepDone) {                             \
            cbfStepDone = true;                         \
            geode::log::info("startup step: {}", name); \
        }                                               \
    } while (false)

$on_mod(Loaded) {
    CBF_STEP("loaded: reading settings");
    buf.console = Mod::get()->getSettingValue<bool>("debug-console");
    buf.showPath = Mod::get()->getSettingValue<bool>("show-path");
    recoverGlobals();
    // v2.58: low latency is switched on for everyone once (it can still be turned off), and
    // the stage you picked last comes back, the same in every level.
    if (!Mod::get()->getSavedValue<bool>("low-latency-v258", false)) {
        Mod::get()->setSettingValue<bool>("low-latency", true);
        Mod::get()->setSavedValue("low-latency-v258", true);
    }
    buf.stage = std::clamp(Mod::get()->getSavedValue<int>("stage", buf.stage), 0, STAGE_COUNT - 1);
    // v2.62: per-level settings off (switched off once): every level uses the same switches
    if (!Mod::get()->getSavedValue<bool>("per-level-v262", false)) {
        Mod::get()->setSettingValue<bool>("per-level-settings", false);
        Mod::get()->setSavedValue("per-level-v262", true);
    }
    CBF_STEP("loaded: settings read, applying stage");
    readSettings();
    applyStage();
    CBF_STEP("loaded: stage applied");
#ifdef GEODE_IS_WINDOWS
    if (buf.console) {
        AllocConsole();
        FILE* out = nullptr;
        freopen_s(&out, "CONOUT$", "w", stdout);
        SetConsoleTitleA("Click Before Frames - Live Console");
    }
#endif
    if (Loader::get()->isModLoaded("syzzi.click_between_frames")) {
        say("Click Between Frames detected: mid-tick sync {}", buf.cbfSync ? "on" : "off");
        cbfRestore();  // test 78: if the game closed while a macro had it off
    }
    if (Loader::get()->isModLoaded("square3ang.cbfextrapolate")) {
        say("CBF Extrapolate detected: visual only, predictions unaffected");
    }
    say("Click Before Frames v3.0 loaded. Stage key cycles: 0.5f -> 1f -> 2f -> 4f -> 6f -> 8f -> custom -> off");
}
