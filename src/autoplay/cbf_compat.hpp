//@ autoplay/cbf_compat.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // test 78: while a macro plays, Click Between Frames is switched off (its "Disable CBF") and so is
    // its Physics Bypass, which changes physics even with CBF disabled: macros are made without them,
    // and a different press or step drifts the macro off. Your settings come back when autoplay stops
    // (or at the next start if the game closed first).
    bool cbfOverridden = false;

    void cbfRestore() {
        auto saved = Mod::get()->getSavedValue<std::string>("cbf-restore", "");
        if (saved.size() != 2) {
            cbfOverridden = false;
            return;
        }
        if (auto* cbf = Loader::get()->getLoadedMod("syzzi.click_between_frames")) {
            if (cbf->hasSetting("soft-toggle")) {
                cbf->setSettingValue<bool>("soft-toggle", saved[0] == '1');
            }
            if (cbf->hasSetting("physics-bypass")) {
                cbf->setSettingValue<bool>("physics-bypass", saved[1] == '1');
            }
            say("Click Between Frames: your settings are back (disabled {}, physics bypass {})", saved[0] == '1' ? "on" : "off",
                saved[1] == '1' ? "on" : "off");
        }
        Mod::get()->setSavedValue("cbf-restore", std::string());
        cbfOverridden = false;
    }

    void cbfOffForMacro() {
        auto* cbf = Loader::get()->getLoadedMod("syzzi.click_between_frames");
        if (!cbf || cbfOverridden || !cbf->hasSetting("soft-toggle")) {
            return;
        }
        bool soft = cbf->getSettingValue<bool>("soft-toggle");
        bool bypass = cbf->hasSetting("physics-bypass") && cbf->getSettingValue<bool>("physics-bypass");
        if (soft && !bypass) {
            return;  // already off
        }
        Mod::get()->setSavedValue("cbf-restore", std::string(soft ? "1" : "0") + (bypass ? "1" : "0"));
        cbfOverridden = true;
        cbf->setSettingValue<bool>("soft-toggle", true);
        if (bypass) {
            cbf->setSettingValue<bool>("physics-bypass", false);
        }
        say("Click Between Frames: switched off while the macro plays{} (back when autoplay stops)", bypass ? ", and its Physics Bypass" : "");
    }
