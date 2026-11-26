//@ route/step_check.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // a tick of autoplay with the route; false = the route has nothing here (the short planner goes on)
    // test 64: what you and the ghost touch, for the step check
    std::string stepWhat(PlayerObject* p) {
        auto obj = [](GameObject* o) {
            return o ? fmt::format("{}@{:.0f},{:.0f}", o->m_objectID, o->getPositionX(), o->getPositionY()) : std::string("-");
        };
        return fmt::format("y {:.2f} vy {:.3f} ground {} slope {} upside {} hit {} on {} slopeObj {}", p->getPositionY(), p->m_yVelocity,
            p->m_isOnGround ? 1 : 0, p->m_isOnSlope ? 1 : 0, p->m_isUpsideDown ? 1 : 0, obj(p->m_collidedObject), obj(p->m_lastGroundObject),
            obj(p->m_currentSlope));
    }

    void stepCheck(PlayLayer* layer, PlayerObject* p1) {
        auto& c = stepChk;
        if (!c.snap || c.tick != tickCount - 1 || p1->m_isDead) {
            return;
        }
        std::vector<int> flips;
        if (c.want != c.held) {
            flips.push_back(c.tick);
        }
        RoutePoint from { c.tick, c.snap, c.held, c.hasB, c.lo, c.hi };
        routeSim(layer, p1, from, flips, 1, false);
        auto* g = buf.ghost1;
        if (!g) {
            return;
        }
        ++c.steps;
        float d = std::max(std::abs(g->getPositionX() - p1->getPositionX()), std::abs(g->getPositionY() - p1->getPositionY()));
        if (d > 0.1f) {
            ++c.off;
        }
        if (d > c.maxD) {
            c.maxD = d;
            c.maxAt = p1->getPosition();
        }
        if (d > 0.5f && c.logged < 24) {
            ++c.logged;
            say("step check tick {} ({}{}): ghost {:.2f} units off after one tick at x {:.0f}", c.tick, MODE_NAMES[modeIndex(p1, nullptr)],
                c.want != c.held ? ", button changed" : "", d, p1->getPositionX());
            say("   you:   {}", stepWhat(p1));
            say("   ghost: {}", stepWhat(g));
            say("   before: {}", stepWhat(c.snap));
        }
        if (tickCount - c.lastSummary >= st(2400)) {
            c.lastSummary = tickCount;
            say("step check: {} ticks, {} off by more than 0.1, most {:.2f} at x {:.0f} y {:.0f}", c.steps, c.off, c.maxD, c.maxAt.x, c.maxAt.y);
            c.maxD = 0.f;
        }
    }

    // test 92: the FPS guard. Your usual frame time is learned (the fastest frames of the last few
    // seconds); while frames take over 15% longer than that, autoplay plans with less time a frame
    // and checks less often, and goes back up slowly once they're quick again
    double fpsScale = 1.0;
    double frameBest = 0.0;
    int frameBestAge = 0;
    double frameAvg = 0.0;
    int fpsLogged = 0;

    void frameGuard(double frameMs) {
        if (frameMs > 200.0) {
            return;  // a hitch (loading, pausing), not the normal speed
        }
        if (frameBest <= 0.0 || frameMs < frameBest || ++frameBestAge > 600) {
            frameBest = frameMs;
            frameBestAge = 0;
        }
        frameAvg = frameAvg <= 0.0 ? frameMs : frameAvg * 0.9 + frameMs * 0.1;
        double before = fpsScale;
        if (frameAvg > frameBest * 1.15) {
            fpsScale = std::max(0.25, fpsScale * 0.92);
        }
        else {
            fpsScale = std::min(1.0, fpsScale * 1.01);
        }
        if (before >= 0.5 && fpsScale < 0.5 && fpsLogged < 5) {
            ++fpsLogged;
            say("autoplay: frames are slow ({:.1f} ms, usually {:.1f}): it plans with less time a frame for now", frameAvg, frameBest);
        }
    }
