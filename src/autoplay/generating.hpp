//@ autoplay/generating.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    void routeReset();
    void makeSnaps(PlayLayer* layer);
    void dropSnaps(bool release);
    void buildObjIndex(PlayLayer* layer);
    void routeCountersReset();

    void endGenerating(char const* result) {
        if (!autoplay.generating) {
            return;
        }
        autoplay.generating = false;
        autoplay.genEnded = std::chrono::steady_clock::now();
        if (autoplay.genCover) {
            autoplay.genCover->removeFromParent();
            autoplay.genCover = nullptr;
            autoplay.genText = nullptr;
        }
        FMODAudioEngine::sharedEngine()->resumeAllAudio();
        double secs = std::chrono::duration<double>(autoplay.genEnded - autoplay.genStart).count();
        say("autoplay path: {} ({} attempts, {:.0f} s, best {:.1f}%)", result, autoplay.genAttempts, secs, autoplay.genBest);
    }

    void beginGenerating(PlayLayer* layer) {
        auto win = CCDirector::get()->getWinSize();
        autoplay.generating = true;
        autoplay.genAttempts = 0;
        autoplay.genBest = 0.f;
        autoplay.genStart = std::chrono::steady_clock::now();
        autoplay.genCover = CCLayerColor::create({ 8, 10, 16, 255 });
        autoplay.genCover->setID("autoplay-making-path"_spr);
        auto* title = CCLabelBMFont::create("Making the path...", "bigFont.fnt");
        title->setScale(0.7f);
        title->setPosition({ win.width / 2.f, win.height / 2.f + 22.f });
        autoplay.genCover->addChild(title);
        autoplay.genText = CCLabelBMFont::create("0%", "goldFont.fnt");
        autoplay.genText->setScale(0.75f);
        autoplay.genText->setPosition({ win.width / 2.f, win.height / 2.f - 12.f });
        autoplay.genCover->addChild(autoplay.genText);
        auto* hint = CCLabelBMFont::create("Autoplay plays the level hidden until it reaches the end. Pause to stop.", "chatFont.fnt");
        hint->setScale(0.6f);
        hint->setOpacity(170);
        hint->setPosition({ win.width / 2.f, win.height / 2.f - 42.f });
        autoplay.genCover->addChild(hint);
        layer->addChild(autoplay.genCover, 999);
        say("autoplay is making the path: it plays the level hidden until the end");
    }

    void updateGenText(PlayLayer* layer) {
        autoplay.genBest = std::max(autoplay.genBest, layer->getCurrentPercent());
        if (autoplay.genText) {
            double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - autoplay.genStart).count();
            autoplay.genText->setString(fmt::format("{:.0f}%  -  {} attempts  -  {:.0f}s", autoplay.genBest, autoplay.genAttempts, secs).c_str());
        }
    }
