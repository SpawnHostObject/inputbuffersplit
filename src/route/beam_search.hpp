//@ route/beam_search.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // ===== v3.0 test 42: the beam search for spots the route is stuck at =====
    // Taking inputs back one at a time can't find a way that needs a different COMBINATION of
    // inputs a few seconds earlier (Stereo Madness 60%: 20 take-backs, same spike every time). From
    // the saved state ~3 s before the death, many input lists are tried side by side: each round
    // every list gets one more input (every 4 ticks after its last one), and the 12 that get
    // furthest (kept varied) go on. A list that gets well past the death becomes the route.
    struct BeamItem {
        std::vector<int> flips;  // absolute ticks (the route's earlier flips included)
        int last;                // the last input's tick (new inputs go after it)
        int end;                 // how far it gets (absolute tick)
    };
    struct Beam {
        bool active = false;
        RoutePoint start {};
        int death = 0;
        int target = 0;
        int depth = 0;
        std::vector<BeamItem> items;
        std::vector<BeamItem> next;
        size_t item = 0;
        int cand = 0;
        int variant = 0;
        int tests = 0;
        int triedFor = INT_MIN;  // the death it last ran for (one run per stuck spot per route)
    };
    Beam beam;
    void beamReset() {
        beam = {};
    }

    bool beamHeldAt(std::vector<int> const& flips, int t) {
        bool held = route.originHeld;
        for (int f : flips) {
            if (f >= t) {
                break;
            }
            held = !held;
        }
        return held;
    }

    // returns true if it started
    bool beamStart(PlayLayer* layer, PlayerObject* p1) {
        if (route.cps.empty() || std::abs(beam.triedFor - route.death) < st(30)) {
            return false;
        }
        beam = {};
        beam.triedFor = route.death;
        beam.death = route.death;
        beam.target = route.death + st(150);  // test 44: shorter tests (was 1 s past the death)
        beam.start = routeCpAtOrBefore(std::max(route.death - st(720), tickCount + 1));
        std::vector<int> base;
        for (int f : route.flips) {
            if (f < beam.start.tick) {
                base.push_back(f);
            }
        }
        int lived = routeSim(layer, p1, beam.start, base, beam.target - beam.start.tick, false);
        int last = std::max(beam.start.tick, tickCount + 1) - st(3);
        beam.items.push_back({ base, last, beam.start.tick + lived });
        beam.cand = last + st(3);
        beam.active = true;
        say("route: stuck {:.1f} s ahead, trying combinations of inputs from {:.1f} s before it", (route.death - tickCount) / static_cast<double>(st(240)),
            (route.death - beam.start.tick) / static_cast<double>(st(240)));
        return true;
    }

    void beamFinish(PlayLayer* layer, BeamItem const& win) {
        // the winning list becomes the route from the beam's start on
        route.flips = win.flips;
        route.groups.erase(std::remove_if(route.groups.begin(), route.groups.end(), [&](RouteGroup const& g) { return g.first >= beam.start.tick; }),
            route.groups.end());
        for (size_t i = 0; i < win.flips.size(); ++i) {
            int f = win.flips[i];
            if (f < beam.start.tick) {
                continue;
            }
            // a tap (press then let go 3 ticks later) is one input
            if (i + 1 < win.flips.size() && win.flips[i + 1] - f <= st(3) && !beamHeldAt(win.flips, f)) {
                route.groups.push_back({ f, win.flips[i + 1] });
                ++i;
            }
            else {
                route.groups.push_back({ f, f });
            }
        }
        route.stuck = false;
        route.sprinting = false;
        route.takebacks = 0;
        route.bannedFirst.clear();
        routeKeepCps(beam.start.tick);
        beam.active = false;
        if (routeReports < 16) {
            ++routeReports;
            say("route: found a way past the stuck spot ({} tests, {} inputs deep)", beam.tests, beam.depth + 1);
        }
    }

    void beamFail(PlayLayer* layer, char const* why) {
        beam.active = false;
        routeStuck(layer, fmt::format("no combination of inputs found either ({}, {} tests)", why, beam.tests));
    }

    // one test
    void beamStep(PlayLayer* layer, PlayerObject* p1) {
        if (beam.item >= beam.items.size()) {
            // the round is over: the furthest 12 go on, at most 2 that end at nearly the same spot
            std::sort(beam.next.begin(), beam.next.end(), [](BeamItem const& a, BeamItem const& b) { return a.end > b.end; });
            std::vector<BeamItem> keep;
            for (auto& it : beam.next) {
                int same = 0;
                for (auto const& k : keep) {
                    same += std::abs(k.end - it.end) <= st(4) ? 1 : 0;
                }
                if (same < 2) {
                    keep.push_back(std::move(it));
                }
                if (keep.size() >= 12) {
                    break;
                }
            }
            beam.next.clear();
            ++beam.depth;
            if (keep.empty()) {
                beamFail(layer, "nothing got further");
                return;
            }
            if (beam.depth >= 10) {
                beamFail(layer, "10 inputs deep");
                return;
            }
            beam.items = std::move(keep);
            beam.item = 0;
            beam.cand = beam.items[0].last + st(3);
            beam.variant = 0;
            return;
        }
        auto const& base = beam.items[beam.item];
        int hi = std::min(base.end - 1, base.last + st(200));
        if (beam.cand > hi) {
            ++beam.item;
            if (beam.item < beam.items.size()) {
                beam.cand = beam.items[beam.item].last + st(3);
                beam.variant = 0;
            }
            return;
        }
        if (beam.cand < tickCount + 2) {
            beam.cand = tickCount + 2;  // time went on while it searched: nothing before now
            return;
        }
        int t = beam.cand;
        auto const& cp = routeCpAtOrBefore(t);
        bool holdMode = cp.snap->m_isDart || cp.snap->m_isShip;
        bool held = beamHeldAt(base.flips, t);
        bool tapMode = !holdMode && !held;
        std::vector<int> f = base.flips;
        f.push_back(t);
        if (tapMode && beam.variant == 0) {
            f.push_back(t + st(3));
        }
        else if (tapMode && beam.variant == 1) {
            f.push_back(t + st(20));  // a longer press (a held jump, let go a little later)
        }
        int lived = routeSim(layer, p1, beam.start, f, beam.target - beam.start.tick, false);
        ++beam.tests;
        int end = beam.start.tick + lived;
        int last = f.back();
        if (end >= beam.target) {
            beamFinish(layer, { f, last, end });
            return;
        }
        if (end > base.end) {
            beam.next.push_back({ f, last, end });
        }
        int variants = tapMode ? 2 : 1;
        if (beam.variant + 1 < variants) {
            ++beam.variant;
        }
        else {
            beam.variant = 0;
            beam.cand += holdMode ? st(3) : st(5);
        }
    }
