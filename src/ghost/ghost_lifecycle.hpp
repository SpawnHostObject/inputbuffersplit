//@ ghost/ghost_lifecycle.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    void dropGhosts() {
#ifdef GEODE_IS_ANDROID
        // v2.87 Android: the ghosts are linked to the level that is closing, and deleting them
        // while it was being torn down crashed on Android (the freed memory is reused at once,
        // so the crash showed up in random other mods: music, labels, the scheduler). They're
        // only let go of here; a hidden player per level is a small cost.
        buf.ghost1 = nullptr;
        buf.ghost2 = nullptr;
        buf.ghostLayer = nullptr;
#else
        for (auto** g : { &buf.ghost1, &buf.ghost2 }) {
            if (*g) {
                (*g)->removeFromParentAndCleanup(true);
                (*g)->release();
                *g = nullptr;
            }
        }
        if (buf.ghostLayer) {
            buf.ghostLayer->removeAllChildrenWithCleanup(true);
            buf.ghostLayer->release();
            buf.ghostLayer = nullptr;
        }
#endif
        // panel nodes are owned by the PlayLayer
        buf.panel = nullptr;
        buf.panelTitle = nullptr;
        buf.panelLines.clear();
        buf.pathNode = nullptr;
        buf.autoTag = nullptr;
        buf.watermark = nullptr;
        buf.simulating = false;
        buf.injecting = false;
        buf.rapid = 0;
        buf.autoReleased = false;
        buf.autoClicked = false;
        buf.recordPath = nullptr;
        watched.clear();
        forgetMoveTriggers();
        drift.active = false;
        resetPending();
        // v2.86: a level you left can't leave a "button down" behind for the next one
        buf.realInput = false;
        buf.appliedInput = false;
        buf.owner = nullptr;
    }

    // PERF: v1 created ghosts with the PlayLayer as their parent layer and put them in
    // the object layer. PlayerObject::init adds ~13 particle systems, 2 motion streaks,
    // a wave trail and a ghost trail to that parent layer, so every frame the game was
    // updating and drawing two invisible players' worth of effects (which also kept
    // emitting wherever the ghost was last simulated). Ghosts now live in a detached
    // layer that is never added to the scene: cocos never visits/draws it and its
    // nodes' scheduled updates stay paused, so the ghosts cost nothing between clicks.
    PlayerObject* makeGhost(PlayLayer* layer) {
        if (!buf.ghostLayer) {
            buf.ghostLayer = CCLayer::create();
            buf.ghostLayer->retain();
        }
        auto* ghost = PlayerObject::create(1, 1, layer, buf.ghostLayer, true);
        ghost->retain();
        ghost->setPosition(CCPoint(0.f, 105.f));
        ghost->setVisible(false);
        buf.ghostLayer->addChild(ghost);
        return ghost;
    }

    // v2.24: anything the ghosts left in their layer since they were made (trail fade-outs,
    // effect nodes...) is removed with cleanup, which also takes it off the scheduler and
    // the action manager. Without this every prediction left a few paused nodes behind,
    // and the game iterates all of them every frame: the FPS sank a little with each save.
    void sweepGhostLayer() {
        if (!buf.ghostLayer) {
            return;
        }
        auto* children = buf.ghostLayer->getChildren();
        while (children && children->count() > buf.ghostLayerBase) {
            auto* node = static_cast<CCNode*>(children->lastObject());
            buf.ghostLayer->removeChild(node, true);
        }
    }
