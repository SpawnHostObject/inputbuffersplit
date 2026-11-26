//@ core/stage.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    void applyStage() {
        auto const& s = STAGES[buf.stage];
        buf.enabled = s.enabled;
        if (s.custom) {
            buf.ticksAll = static_cast<int>(Mod::get()->getSettingValue<int64_t>("custom-ticks"));
            buf.ticksWave = static_cast<int>(Mod::get()->getSettingValue<int64_t>("custom-wave-ticks"));
        }
        else {
            buf.ticksAll = s.ticksAll;
            buf.ticksWave = s.ticksWave;
        }
    }
