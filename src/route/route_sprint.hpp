//@ route/route_sprint.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // v3.0 test 34: where the route got stuck (by level x, 60 units a slot) and how often. Kept for
    // the level while autoplay is on: each time it's stuck at the same spot again, the next search
    // there goes deeper (more take-backs, further back), so attempts make progress instead of
    // getting stuck the same way every time

    float routeXAt(int tick) {
        int k = tick - route.posBase - 1;
        if (k >= 0 && k < static_cast<int>(route.pos.size())) {
            return route.pos[k].x;
        }
        return route.pos.empty() ? 0.f : route.pos.back().x;
    }

    int routeStuckHere(int tick) {
        int slot = static_cast<int>(routeXAt(tick) / 60.f);
        int n = 0;
        for (int d = -1; d <= 1; ++d) {
            auto it = routeStuckSpots.find(slot + d);
            if (it != routeStuckSpots.end()) {
                n = std::max(n, it->second);
            }
        }
        return std::min(n, 6);
    }

    void routeStuck(PlayLayer* layer, std::string why) {
        ++routeStuckSpots[static_cast<int>(routeXAt(route.death) / 60.f)];
        route.sprinting = false;
        route.stuck = true;
        route.stuckAt = route.death;
        route.why = why;
        autoplay.killer = "route: " + why;
        if (routeReports < 16) {
            ++routeReports;
            say("route stuck {:.1f} s ahead: {}", (route.death - tickCount) / static_cast<double>(st(240)), why);
            // v3.0 test 41: what it was doing there, to see which move it never tries
            int k = route.death - route.posBase - 1;
            CCPoint at = k >= 0 && k < static_cast<int>(route.pos.size()) ? route.pos[k] : CCPoint {};
            auto const& cp = routeCpAtOrBefore(route.death);
            char const* mode = cp.snap->m_isShip ? "ship" : cp.snap->m_isDart ? "wave" : cp.snap->m_isBall ? "ball" : cp.snap->m_isBird ? "UFO"
                : cp.snap->m_isRobot ? "robot" : cp.snap->m_isSpider ? "spider" : cp.snap->m_isSwing ? "swing" : "cube";
            int orbsNear = 0;
            std::string orbList;
            int b = static_cast<int>(std::floor(at.x / 64.f));
            for (int kk = b - 8; kk <= b + 2; ++kk) {
                auto it = buf.objIndex.find(kk);
                if (it == buf.objIndex.end()) {
                    continue;
                }
                for (auto* o : it->second) {
                    if (typeinfo_cast<RingObject*>(o) && o->m_positionX > at.x - 500.f && o->m_positionX < at.x + 120.f) {
                        ++orbsNear;
                        if (orbsNear <= 6) {
                            orbList += fmt::format(" {}@{:.0f},{:.0f}", o->m_objectID, o->m_positionX, o->m_positionY);
                        }
                    }
                }
            }
            say("  stuck detail: dies at x {:.0f} y {:.0f} as {} (button {} before it); searched back to {:.2f} s before, {} tests; "
                "furthest any input got: {} ticks past it; orbs near (id@x,y): {}{}; touched on the way: {}; orb clicks tried {}, best {} ticks past",
                at.x, at.y, mode, routeHeldAt(route.death) ? "held" : "let go",
                route.lowest == INT_MAX ? 0.0 : (route.death - route.lowest) / static_cast<double>(st(240)), route.sprintTests,
                route.bestEnd - route.death, orbsNear, orbList, route.orbRanges.size(), route.orbTries, route.orbBest);
        }
    }

    void routeBeginSprint(int death) {
        route.sprinting = true;
        route.death = death;
        int lastFlip = route.flips.empty() ? -1000000 : route.flips.back();
        route.lower = std::max({ death - st(240 + 60 * routeStuckHere(death)), lastFlip + 1, tickCount + 1, route.cps.front().tick });
        // v3.0 test 32: right after a take-back, only inputs BEFORE the one taken back: test 31 put
        // nearly the same input back a tick or two off (a wave turn at the same peak, 8 times over)
        route.cand = std::min(death - 1, route.beforeLimit - st(2));
        route.beforeLimit = INT_MAX;
        route.candStart = route.cand;
        route.variant = 0;
        route.windowHi = -1;
        route.fine = false;
        route.bestT = -1;
        route.bestEnd = death;
        route.orbT = -1;
        route.orbEnd = 0;
        route.sprintTests = 0;
        route.orbTries = 0;
        route.orbBest = 0;
        route.lowest = INT_MAX;
        route.orbFirst.clear();
        route.orbRanges.clear();
    }

    // v3.0 test 39: a click while touching an orb only has to get a little past the death: in an
    // orb chain each click only lives until the next orb (which gets its own search), so the 0.5 s
    // rule never let one through (60% of Stereo Madness)
    int routeNeeded(int t) {
        for (auto const& [a, b] : route.orbRanges) {
            if (t >= a - st(2) && t <= b) {
                return st(30);
            }
        }
        return st(120);
    }

    int routeCoarseStep(int t) {
        auto const& cp = routeCpAtOrBefore(t);
        return (cp.snap->m_isDart || cp.snap->m_isShip) ? st(2) : st(3);
    }

    // a flip group at t: variant 0 outside wave / ship while let go is a tap, otherwise a flip
    std::vector<int> routeWith(int t, bool tap) {
        std::vector<int> f = route.flips;
        f.push_back(t);
        if (tap) {
            f.push_back(t + st(3));
        }
        return f;
    }

    // v3.0 test 55: still holding outside wave / ship: an orb or a jump needs a new click (let go,
    // press again a tick later, let go after 4). The route only ever let go there, so an orb reached
    // while holding was never clicked (Aquamarine 33.5%: "orb clicks tried 0", button held).
    bool routeReclickMode(int t) {
        auto const& cp = routeCpAtOrBefore(t);
        bool holdMode = cp.snap->m_isDart || cp.snap->m_isShip;
        return !holdMode && routeHeldAt(t);
    }

    std::vector<int> routeReclick(int t) {
        std::vector<int> f = route.flips;
        f.push_back(t);
        f.push_back(t + 1);
        f.push_back(t + 1 + st(3));
        return f;
    }

    bool routeTapMode(int t) {
        auto const& cp = routeCpAtOrBefore(t);
        bool holdMode = cp.snap->m_isDart || cp.snap->m_isShip;
        return !holdMode && !routeHeldAt(t);
    }

    void routeAccept(PlayLayer* layer, PlayerObject* p1, int hi, int lo, int variant) {
        int t = (hi + lo) / 2;  // the middle of the timings that work: room on both sides
        bool tapMode = routeTapMode(t);
        bool tap = variant == 0 && tapMode;
        if (variant == 1 && routeReclickMode(t)) {
            route.flips.push_back(t);
            route.flips.push_back(t + 1);
            route.flips.push_back(t + 1 + st(3));
            route.groups.push_back({ t, t + 1 + st(3) });
            route.sprinting = false;
            route.afterTakeback = false;
            routeKeepCps(t);
            return;
        }
        route.flips.push_back(t);
        int last = t;
        if (tap) {
            last = t + st(3);
            route.flips.push_back(last);
        }
        else if (tapMode) {
            // v3.0 test 37: a held press outside wave / ship (it jumps again on every landing while
            // held, and nothing ever let go: the cube hopped along flat ground for no reason). Let go
            // at the first of these that still gets as far as holding does.
            auto const& cp = routeCpAtOrBefore(t);
            int length = route.death - cp.tick + st(120);
            int held = routeSim(layer, p1, cp, route.flips, length, false);
            for (int r : { st(10), st(20), st(35), st(60) }) {
                auto f = route.flips;
                f.push_back(t + r);
                if (routeSim(layer, p1, cp, f, length, false) >= std::min(held, length)) {
                    last = t + r;
                    route.flips.push_back(last);
                    break;
                }
            }
        }
        route.groups.push_back({ t, last });
        route.sprinting = false;
        route.afterTakeback = false;
        routeKeepCps(t);
    }
