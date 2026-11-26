//@ ui/cbf_icon.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // test 91: the CBF icon, sized to a game button
    CCSprite* cbfIcon(float size) {
        auto* s = CCSprite::create("cbf-icon.png"_spr);
        if (s && s->getContentSize().width > 0.f) {
            s->setScale(size / s->getContentSize().width);
        }
        return s;
    }

    void manualUpdate();
    std::string updateButtonText();
    bool updateAvailable();
    std::string levelFingerprint(GJGameLevel* level);
    void brainFetch(int levelId, std::string print, int forceTps = 0);

    int macroOffsetSetting = 0;  // test 93: read once an attempt
