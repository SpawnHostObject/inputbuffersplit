//@ route/pathfinder.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // ===== v3.0 test 57/58: the pathfinder (wave / ship) =====
    // When the route is stuck in wave or ship, many paths are followed side by side: every step
    // (6 ticks) each path either keeps the button or flips it (at the start of the step or halfway;
    // if all of those die, at every other tick of the step too), and its hidden player's state is
    // saved at the end of the step. Up to 8 living paths at different heights go on (the route's
    // own path is always one of them). Test 58: it runs while you play. As you come close, the
    // inputs of the path in the middle of the others are given to you, step by step, and only the
    // paths that agree with them go on. The first path that gets well past the death finishes it.
    struct PfNode {
        PlayerObject* snap = nullptr;
        bool held = false;
        bool onRoute = false;
        int tick = 0;
        std::vector<int> flips;  // from pf.start on
        bool hasB = false;
        float lo = 0.f, hi = 0.f;
        float y = 0.f;
    };
    struct Pathfinder {
        bool active = false;
        int origStart = 0;
        int start = 0;
        int death = 0;
        int target = 0;
        std::vector<int> base;  // inputs before start (given to you)
        std::vector<int> orig;  // the old route's inputs from origStart on
        std::vector<PfNode> frontier;
        std::vector<PfNode> next;
        size_t idx = 0;
        int variant = 0;
        int childrenOfNode = 0;
        int tests = 0;
        int steps = 0;
        int commits = 0;
        double ms = 0.0;
        int triedFor = INT_MIN;
    };
    Pathfinder pf;
    constexpr int PF_WIDTH = 8;

    void pfGive(PfNode& n) {
        if (n.snap) {
            pfFree.push_back(n.snap);
            n.snap = nullptr;
        }
    }

    void pfReset() {
        for (auto& n : pf.frontier) {
            pfGive(n);
        }
        for (auto& n : pf.next) {
            pfGive(n);
        }
        int tried = pf.triedFor;
        pf = {};
        pf.triedFor = tried;
    }

    bool pfStart(PlayLayer* layer, PlayerObject* p1) {
        if (route.cps.empty() || std::abs(pf.triedFor - route.death) < st(30) || pfFree.size() < 8) {
            return false;
        }
        auto const& dcp = routeCpAtOrBefore(route.death);
        if (!(dcp.snap->m_isDart || dcp.snap->m_isShip)) {
            return false;  // cube-like parts keep the jump search
        }
        pfReset();
        pf.triedFor = route.death;
        auto const& cp = routeCpAtOrBefore(std::max(route.death - st(720), tickCount + 2));
        if (cp.tick < tickCount + st(12)) {
            return false;  // test 59: its start would be behind you already (it "fell behind" at once)
        }
        auto* s = pfFree.back();
        pfFree.pop_back();
        copyState(s, cp.snap);
        PfNode root;
        root.snap = s;
        root.held = cp.held;
        root.onRoute = true;
        root.tick = cp.tick;
        root.hasB = cp.hasBounds;
        root.lo = cp.lo;
        root.hi = cp.hi;
        root.y = s->getPositionY();
        pf.origStart = pf.start = cp.tick;
        pf.death = route.death;
        pf.target = route.death + st(150);
        for (int f : route.flips) {
            (f < cp.tick ? pf.base : pf.orig).push_back(f);
        }
        pf.frontier.push_back(std::move(root));
        pf.active = true;
        if (routeReports < 16) {
            ++routeReports;
            say("route: stuck {:.1f} s ahead in {}, pathfinder from {:.1f} s before it", (route.death - tickCount) / static_cast<double>(st(240)),
                dcp.snap->m_isDart ? "wave" : "ship", (route.death - cp.tick) / static_cast<double>(st(240)));
        }
        return true;
    }

    void pfFinish(PlayLayer* layer, PfNode const& win) {
        route.flips = pf.base;
        route.flips.insert(route.flips.end(), win.flips.begin(), win.flips.end());
        int from = pf.origStart;
        route.groups.erase(std::remove_if(route.groups.begin(), route.groups.end(), [&](RouteGroup const& g) { return g.first >= from; }),
            route.groups.end());
        for (int f : route.flips) {
            if (f >= from) {
                route.groups.push_back({ f, f });
            }
        }
        route.stuck = false;
        route.sprinting = false;
        route.takebacks = 0;
        route.bannedFirst.clear();
        if (routeReports < 16) {
            ++routeReports;
            say("route: pathfinder found a way ({} steps, {} tests, {:.2f} ms each, {} given to you on the way)", pf.steps, pf.tests,
                pf.ms / std::max(1, pf.tests), pf.commits);
        }
        pfReset();
        routeKeepCps(from);
    }

    void pfFail(PlayLayer* layer, char const* why) {
        auto msg = fmt::format("the pathfinder {} ({} steps, {} tests, {:.2f} ms each)", why, pf.steps, pf.tests, pf.ms / std::max(1, pf.tests));
        // what was given to you stays (nothing given: the old route stays)
        if (pf.commits > 0) {
            route.flips = pf.base;
        }
        pfReset();
        routeStuck(layer, msg);
    }

    // the path in the middle of the others (the most room on both sides)
    size_t pfMiddle(std::vector<PfNode> const& v) {
        float lo = v.front().y, hi = v.front().y;
        for (auto const& n : v) {
            lo = std::min(lo, n.y);
            hi = std::max(hi, n.y);
        }
        float mid = (lo + hi) * 0.5f;
        size_t best = 0;
        for (size_t i = 1; i < v.size(); ++i) {
            if (std::abs(v[i].y - mid) < std::abs(v[best].y - mid)) {
                best = i;
            }
        }
        return best;
    }

    // the flips a variant puts in a step starting at t (-1 = no such variant)
    bool pfVariant(PfNode const& node, int v, int step, std::vector<int>& out) {
        out.clear();
        int t = node.tick;
        static int const offs[] = { 0, 3, 1, 2, 4, 5 };
        if (v == 0) {
            return true;  // keep the button
        }
        if (v >= 1 && v <= 6) {
            int o = st(offs[v - 1]);
            if (v >= 3 && (o == st(offs[0]) || o == st(offs[1]))) {
                return false;  // same tick as a coarse one (low tick rates)
            }
            if (o >= step) {
                return false;
            }
            out.push_back(t + o);
            return true;
        }
        if (v == 7) {
            // the old route's own inputs in this step, if none of the others is the same
            if (!node.onRoute) {
                return false;
            }
            for (int f : pf.orig) {
                if (f >= t && f < t + step) {
                    out.push_back(f);
                }
            }
            if (out.empty()) {
                return false;
            }
            if (out.size() == 1 && (out[0] == t || out[0] == t + st(3))) {
                return false;
            }
            return true;
        }
        return false;
    }

    bool pfMatchesOrig(PfNode const& node, std::vector<int> const& flips, int step) {
        std::vector<int> want;
        for (int f : pf.orig) {
            if (f >= node.tick && f < node.tick + step) {
                want.push_back(f);
            }
        }
        return want == flips;
    }

    void pfStep(PlayLayer* layer, PlayerObject* p1) {
        int step = st(6);
        if (pf.idx < pf.frontier.size()) {
            auto& node = pf.frontier[pf.idx];
            int v = pf.variant;
            std::vector<int> flips;
            bool have = v <= 7 && pfVariant(node, v, step, flips);
            if (have) {
                RoutePoint from { node.tick, node.snap, node.held, node.hasB, node.lo, node.hi };
                pfOut = nullptr;
                pfStepLen = step;
                pfActive = true;
                auto t0 = std::chrono::steady_clock::now();
                int lived = routeSim(layer, p1, from, flips, step, true);
                pf.ms += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
                pfActive = false;
                ++pf.tests;
                if (pfOut && lived >= step) {
                    PfNode child;
                    child.snap = pfOut;
                    child.held = pfOutHeld;
                    child.onRoute = node.onRoute && pfMatchesOrig(node, flips, step);
                    child.tick = node.tick + step;
                    child.flips = node.flips;
                    child.flips.insert(child.flips.end(), flips.begin(), flips.end());
                    child.hasB = pfOutB;
                    child.lo = pfOutLo;
                    child.hi = pfOutHi;
                    child.y = pfOut->getPositionY();
                    pf.next.push_back(std::move(child));
                    ++pf.childrenOfNode;
                }
                else if (pfOut) {
                    pfFree.push_back(pfOut);
                }
                pfOut = nullptr;
            }
            // next variant: 0, 1, 2, then 3..6 only if nothing lived, then 7 (the old route's)
            if (v == 2 && pf.childrenOfNode > 0) {
                pf.variant = 7;
            }
            else if (v < 7) {
                pf.variant = v + 1;
            }
            else {
                pf.variant = 8;
            }
            if (pf.variant > 7) {
                pf.variant = 0;
                pf.childrenOfNode = 0;
                ++pf.idx;
            }
            return;
        }
        // the round is over: the old paths go, the new ones are thinned out (one per 6 units of
        // height and button state, the route's own path first, at most 8 spread over the heights)
        for (auto& n : pf.frontier) {
            pfGive(n);
        }
        pf.frontier.clear();
        ++pf.steps;
        std::sort(pf.next.begin(), pf.next.end(), [](PfNode const& a, PfNode const& b) { return a.y < b.y; });
        std::vector<PfNode> kept;
        std::set<int> seenKeys;
        auto key = [](PfNode const& n) { return static_cast<int>(std::floor(n.y / 6.f)) * 2 + (n.held ? 1 : 0); };
        for (int pass = 0; pass < 2; ++pass) {
            for (auto& n : pf.next) {
                if (!n.snap || n.onRoute != (pass == 0)) {
                    continue;
                }
                if (seenKeys.insert(key(n)).second) {
                    kept.push_back(std::move(n));
                    n.snap = nullptr;
                }
                else {
                    pfGive(n);
                }
            }
        }
        pf.next.clear();
        std::sort(kept.begin(), kept.end(), [](PfNode const& a, PfNode const& b) { return a.y < b.y; });
        if (kept.size() > static_cast<size_t>(PF_WIDTH)) {
            std::vector<PfNode> spread;
            for (size_t i = 0; i < static_cast<size_t>(PF_WIDTH); ++i) {
                size_t j = i * (kept.size() - 1) / (PF_WIDTH - 1);
                if (kept[j].snap) {
                    spread.push_back(std::move(kept[j]));
                    kept[j].snap = nullptr;
                }
            }
            for (auto& n : kept) {
                if (n.onRoute && n.snap) {
                    spread.push_back(std::move(n));
                    n.snap = nullptr;
                }
                pfGive(n);
            }
            kept = std::move(spread);
        }
        if (kept.empty()) {
            pfFail(layer, "found no way either");
            return;
        }
        int at = kept.front().tick;
        if (at >= pf.target) {
            PfNode win = std::move(kept[pfMiddle(kept)]);
            for (auto& n : kept) {
                if (n.snap != win.snap) {
                    pfGive(n);
                }
                n.snap = nullptr;
            }
            pfFinish(layer, win);
            pfGive(win);
            return;
        }
        // test 58: you're coming close: the next step of the middle path is given to you
        while (tickCount + st(48) >= pf.start && pf.start + step <= at) {
            auto const& pick = kept[pfMiddle(kept)];
            std::vector<int> part;
            for (int f : pick.flips) {
                if (f < pf.start + step) {
                    part.push_back(f);
                }
            }
            std::vector<PfNode> agree;
            for (auto& n : kept) {
                std::vector<int> mine;
                for (int f : n.flips) {
                    if (f < pf.start + step) {
                        mine.push_back(f);
                    }
                }
                if (mine == part) {
                    n.flips.erase(n.flips.begin(), n.flips.begin() + static_cast<long>(part.size()));
                    agree.push_back(std::move(n));
                    n.snap = nullptr;
                }
                else {
                    pfGive(n);
                }
            }
            kept = std::move(agree);
            pf.base.insert(pf.base.end(), part.begin(), part.end());
            pf.start += step;
            ++pf.commits;
            route.flips = pf.base;
        }
        if (tickCount + st(2) >= at) {
            pf.frontier = std::move(kept);
            pfFail(layer, "fell behind you");
            return;
        }
        pf.frontier = std::move(kept);
        pf.idx = 0;
        pf.variant = 0;
        pf.childrenOfNode = 0;
    }
