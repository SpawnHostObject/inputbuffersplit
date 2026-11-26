//@ route/snapshots.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    constexpr int SNAP_MAX = 48;
    std::vector<PlayerObject*> snapFree;
    std::vector<PlayerObject*> snapAll;
    CCLayer* snapLayer = nullptr;
    PlayLayer* snapOwner = nullptr;

    // the saved states are hidden players of their own, made once per level when autoplay starts
    // test 57: the pathfinder's own pool of saved states
    constexpr int PF_SNAPS = 64;
    std::vector<PlayerObject*> pfFree;
    // test 64: the step check (your state at the start of a tick, played one tick by the ghost)
    struct StepCheck {
        PlayerObject* snap = nullptr;
        int tick = -1;
        bool held = false;
        bool want = false;
        bool hasB = false;
        float lo = 0.f, hi = 0.f;
        int steps = 0;
        int off = 0;
        int logged = 0;
        float maxD = 0.f;
        CCPoint maxAt {};
        int lastSummary = 0;
    };
    StepCheck stepChk;
    void makeSnaps(PlayLayer* layer) {
        if (snapOwner != layer) {
            stepChk = {};
            pfFree.clear();
            snapFree.clear();  // another level's: let go of in onQuit (or left, see dropSnaps)
            snapAll.clear();
            snapLayer = nullptr;
            snapOwner = layer;
        }
        if (!snapLayer) {
            snapLayer = CCLayer::create();
            snapLayer->retain();
        }
        while (snapAll.size() < static_cast<size_t>(SNAP_MAX + PF_SNAPS)) {
            auto* s = PlayerObject::create(1, 1, layer, snapLayer, true);
            s->retain();
            s->setVisible(false);
            snapLayer->addChild(s);
            snapAll.push_back(s);
            if (snapFree.size() < static_cast<size_t>(SNAP_MAX)) {
                snapFree.push_back(s);
            }
            else {
                pfFree.push_back(s);
            }
        }
        if (!stepChk.snap && !pfFree.empty()) {
            stepChk.snap = pfFree.back();
            pfFree.pop_back();
        }
    }

    void dropSnaps(bool release) {
#ifdef GEODE_IS_ANDROID
        release = false;  // like the ghosts (v2.87): never deleted while a level closes on Android
#endif
        if (release) {
            for (auto* s : snapAll) {
                s->removeFromParentAndCleanup(true);
                s->release();
            }
            if (snapLayer) {
                snapLayer->removeAllChildrenWithCleanup(true);
                snapLayer->release();
            }
        }
        snapAll.clear();
        snapFree.clear();
        pfFree.clear();
        stepChk = {};
        snapLayer = nullptr;
        snapOwner = nullptr;
    }

    // test 57: the pathfinder's own saved states (kept apart from the route's)
    bool pfActive = false;
    int pfStepLen = 0;
    PlayerObject* pfOut = nullptr;
    bool pfOutHeld = false;
    bool pfOutB = false;
    float pfOutLo = 0.f, pfOutHi = 0.f;

    void pfSink(int rel, PlayerObject* ghost, bool held) {
        if (rel != pfStepLen || pfOut || pfFree.empty()) {
            return;
        }
        pfOut = pfFree.back();
        pfFree.pop_back();
        copyState(pfOut, ghost);
        pfOutHeld = held;
        pfOutB = buf.gBounds;
        pfOutLo = buf.gBoundLo;
        pfOutHi = buf.gBoundHi;
    }

    PlayerObject* takeSnap() {
        if (snapFree.empty()) {
            return nullptr;
        }
        auto* s = snapFree.back();
        snapFree.pop_back();
        return s;
    }
