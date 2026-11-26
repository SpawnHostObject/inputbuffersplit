//@ autoplay/on_finished.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // v3.0 test 24: a run to the end: the whole level is known and saved
    void autoplayFinished(PlayLayer* layer) {
        if (!autoplay.active || autoplay.layer != layer || layer->m_isPracticeMode || autoplay.userTouched) {
            return;
        }
        autoplay.known = autoplay.attemptFlips;
        autoplay.knownTrail = autoplay.attemptTrail;
        autoplay.knownUntil = autoplay.attemptTick + st(240);
        autoplay.knowsWholeLevel = true;
        saveAutoplay();
        drawTrail();
        say("autoplay finished the level: the whole run is saved, it plays it the same from the first attempt next time");
        if (!layer->m_startPosObject) {
            if (autoplay.macroIsBrain) {
                brainResult(true);  // v3.1.1: the brain's run finished here: it ranks higher
            }
            brainUpload(layer);  // test 90
        }
        if (autoplay.generating) {
            endGenerating("reached the end");
            autoplay.needRestart = true;  // now the run you see, along the trail
        }
    }
