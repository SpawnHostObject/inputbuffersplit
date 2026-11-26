//@ route/route_search.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    void routeTakeBack(PlayLayer* layer, PlayerObject* p1) {
        int deeper = routeStuckHere(route.death);
        if (route.groups.empty() || route.takebacks >= 8 + 6 * deeper || route.groups.back().first <= tickCount + 1) {
            if (pfStart(layer, p1) || beamStart(layer, p1)) {  // test 57: the pathfinder in wave / ship
                route.sprinting = false;
                return;
            }
            routeStuck(layer, fmt::format("no input in the second before it gets through ({} taken back, {} times stuck here before)",
                                  route.takebacks, deeper));
            return;
        }
        auto g = route.groups.back();
        route.groups.pop_back();
        // v3.0 test 37: timings within 3 ticks of the one that failed aren't tried again; earlier
        // AND later ones are (test 32-36 only looked earlier, and a jump that had to be a bit later
        // was never found: Stereo Madness 20%)
        for (int d = -st(10); d <= st(10); ++d) {  // test 40: was 3 (it walked one jump down 7 ticks at a time)
            route.bannedFirst.insert(g.first + d);
        }
        route.afterTakeback = true;
        while (!route.flips.empty() && route.flips.back() >= g.first) {
            route.flips.pop_back();
        }
        ++route.takebacks;
        route.sprinting = false;
        routeKeepCps(g.first);
    }

    // one test of the search. Latest first: an input that gets well past the death is taken (the
    // middle of the timings that do). v3.0 test 29: none does -> the one that got furthest is
    // taken anyway, and the next death gets its own search (test 28 wanted one input to clear
    // everything up to 0.2 s past the death, so a part that needs two quick inputs was "stuck")
    void routeSprintStep(PlayLayer* layer, PlayerObject* p1) {
        if (!route.orbFirst.empty() && route.windowHi < 0) {
            int t = route.orbFirst.front();
            route.orbFirst.erase(route.orbFirst.begin());
            if (t >= route.lower && t < route.death && !route.bannedFirst.count(t)) {
                bool tapMode = routeTapMode(t);
                bool reclick = routeReclickMode(t);
                auto const& cp = routeCpAtOrBefore(t);
                int cpTick = cp.tick;
                int length = route.death - cpTick + routeNeeded(t);
                int lived = routeSim(layer, p1, cp, reclick ? routeReclick(t) : routeWith(t, tapMode), length, false);
                ++route.orbTries;
                route.orbBest = std::max(route.orbBest, cpTick + lived - route.death);
                if (lived >= length) {
                    routeAccept(layer, p1, t, t, reclick ? 1 : 0);  // the orb's middle gets through: taken as it is
                    return;
                }
                if (cpTick + lived > route.bestEnd) {
                    route.bestEnd = cpTick + lived;
                    route.bestT = t;
                    route.bestV = 0;
                }
            }
            return;
        }
        if (route.cand < route.lower) {
            if (route.windowHi >= 0) {
                routeAccept(layer, p1, route.windowHi, route.lower, route.windowVariant);
            }
            else if (route.orbT >= 0) {
                routeAccept(layer, p1, route.orbT, route.orbT, route.orbV);  // test 38: toward the orb
            }
            else if (route.bestT >= 0 && route.bestEnd >= route.death + st(6)) {
                routeAccept(layer, p1, route.bestT, route.bestT, route.bestV);
            }
            else if (!route.fine) {
                route.fine = true;  // the quick pass (every 2-3 ticks) found nothing: now every tick
                route.cand = route.candStart;
                route.variant = 0;
            }
            else if (route.bandOffUntil < route.death) {
                // test 45: nothing works inside the band here: this stretch may go anywhere
                auto const& cp = routeCpAtOrBefore(route.death);
                if (cp.snap->m_isDart || cp.snap->m_isShip || route.death - route.cps.front().tick < st(720)) {
                    route.bandOffUntil = route.death + st(240);
                    route.sprinting = false;
                    routeKeepCps(routeCpAtOrBefore(std::max(route.death - st(480), route.cps.front().tick)).tick);
                }
                else {
                    routeTakeBack(layer, p1);
                }
            }
            else {
                routeTakeBack(layer, p1);
            }
            return;
        }
        int t = route.cand;
        int step = route.fine ? 1 : routeCoarseStep(t);
        if (route.bannedFirst.count(t)) {
            route.cand -= step;
            route.variant = 0;
            return;
        }
        bool tapMode = routeTapMode(t);
        bool reclickMode = routeReclickMode(t);
        int variants = (tapMode || reclickMode) ? 2 : 1;
        int v = route.windowHi >= 0 ? route.windowVariant : route.variant;
        auto flips = (reclickMode && v == 1) ? routeReclick(t) : routeWith(t, tapMode && v == 0);
        auto const& cp = routeCpAtOrBefore(t);
        int cpTick = cp.tick;
        int length = route.death - cpTick + routeNeeded(t);
        int lived = routeSim(layer, p1, cp, flips, length, false);
        bool ok = lived >= length;
        ++route.sprintTests;
        route.lowest = std::min(route.lowest, t);
        if (!ok) {
            // v3.0 test 38: does this input's path reach an orb (touched after the input)? Then the
            // next search can click that orb (51% of Stereo Madness: a jump, then the orb above the
            // pillar; the jump alone dies, so it never looked like the way)
            for (auto const& [ring, touched] : buf.orbTouches) {
                if (cpTick + touched > t && cpTick + lived >= route.death && cpTick + lived > route.orbEnd) {
                    route.orbT = t;
                    route.orbV = v;
                    route.orbEnd = cpTick + lived;
                    break;
                }
            }
        }
        if (cpTick + lived > route.bestEnd) {
            route.bestEnd = cpTick + lived;
            route.bestT = t;
            route.bestV = v;
        }
        if (route.windowHi >= 0) {
            // measuring the window of timings that work. v3.0 test 36: the WHOLE window (up to
            // 0.25 s, 2 ticks a step): test 35 stopped after 8 ticks, so "the middle" was still
            // the last moment possible and every easy jump came out late and frame perfect
            int wstep = st(2);
            if (!ok) {
                routeAccept(layer, p1, route.windowHi, std::min(route.windowHi, t + wstep), route.windowVariant);
            }
            else if (route.windowHi - t >= st(60) || t - wstep < route.lower) {
                routeAccept(layer, p1, route.windowHi, t, route.windowVariant);
            }
            else {
                route.cand -= wstep;
            }
            return;
        }
        if (ok && route.afterTakeback) {
            routeAccept(layer, p1, t, t, v);  // test 37: right after a take-back: this timing itself
            return;
        }
        if (ok && !route.fine) {
            // the quick pass found one: tick by tick from just above it, for the latest
            route.fine = true;
            route.variant = v;
            route.cand = std::min(t + step - 1, route.candStart);
            if (route.cand == t) {
                route.windowHi = t;
                route.windowVariant = v;
                --route.cand;
            }
            return;
        }
        if (ok) {
            route.windowHi = t;
            route.windowVariant = v;
            --route.cand;
            return;
        }
        if (v + 1 < variants) {
            route.variant = v + 1;
        }
        else {
            route.variant = 0;
            route.cand -= step;
        }
    }

    // grow the route at its end; false when there's nothing to do
    bool routeExtend(PlayLayer* layer, PlayerObject* p1) {
        if (route.stuck || route.cps.size() >= static_cast<size_t>(SNAP_MAX) - 4) {
            return false;
        }
        auto from = route.cps.back();
        int length = std::min(st(240), tickCount + st(2400) - from.tick);
        if (length < st(12)) {
            return false;
        }
        // v3.0 test 38: still holding in a mode where holding means jumping again on every landing
        // (a hold from the ship carried through a cube portal: it hopped along for no reason). Let
        // go here if that does no worse.
        if (from.held && !(from.snap->m_isDart || from.snap->m_isShip) && routeHeldAt(from.tick + 1) == from.held) {
            int r = std::max(from.tick, tickCount + 1);
            bool flipAtR = std::find(route.flips.begin(), route.flips.end(), r) != route.flips.end();
            if (!flipAtR && (route.flips.empty() || route.flips.back() < r)) {
                int keep = routeSim(layer, p1, from, route.flips, length, false);
                auto f = route.flips;
                f.push_back(r);
                if (routeSim(layer, p1, from, f, length, false) >= keep) {
                    route.flips.push_back(r);
                    route.groups.push_back({ r, r });
                }
            }
        }
        int lived = routeSim(layer, p1, from, route.flips, length, true);
        for (auto const& c : routeNewCps) {
            route.cps.push_back(c);
        }
        route.pos.resize(std::max(0, from.tick - route.posBase));
        route.pos.insert(route.pos.end(), routeRec.begin(), routeRec.end());
        int got = static_cast<int>(routeRec.size());
        route.changed = true;
        if (lived >= length && got >= length) {
            route.end = from.tick + length;
            if (route.end > route.frontier + st(120)) {
                route.takebacks = 0;  // well past the last hard spot: a new one gets its own 8
            }
            route.frontier = std::max(route.frontier, route.end);
            return true;
        }
        if (lived >= length) {
            // it stopped at something it can't go through (a portal it doesn't simulate)
            route.end = from.tick + got;
            route.death = route.end;
            routeStuck(layer, "a portal the hidden player can't take");
            return false;
        }
        route.end = from.tick + lived;
        route.frontier = std::max(route.frontier, route.end);
        std::vector<std::pair<int, int>> orbs;  // (first, last) touch ticks, absolute
        for (size_t i = 0; i < buf.orbTouches.size() && i < buf.orbTouchLast.size(); ++i) {
            orbs.push_back({ from.tick + buf.orbTouches[i].second, from.tick + buf.orbTouchLast[i] });
        }
        routeBeginSprint(from.tick + lived);
        route.orbRanges = orbs;
        // v3.0 test 35: orbs it went through on the way: a click in the middle of each is tried
        // first, the nearest to the death first (orb clicks came out too early / late / missing)
        for (auto it = orbs.rbegin(); it != orbs.rend(); ++it) {
            int mid = (it->first + it->second) / 2;
            if (mid >= route.lower && mid < route.death) {
                route.orbFirst.push_back(mid);
            }
        }
        return true;
    }

    void routeDraw(PlayerObject* p1) {
        auto* d = autoplay.trailNode;
        if (!d || (!route.changed && tickCount - route.drawnAt < st(6))) {  // test 49: redrawn ~40x a second
            return;
        }
        route.changed = false;
        route.drawnAt = tickCount;
        d->clear();
        int from = std::max(0, tickCount - route.posBase);
        CCPoint prev = p1->getPosition();
        int step = std::max(1, st(4));
        for (int k = from; k < static_cast<int>(route.pos.size()); k += step) {
            auto const& pt = route.pos[k];
            if (std::abs(pt.x - prev.x) < 150.f && std::abs(pt.y - prev.y) < 150.f) {
                d->drawSegment(prev, pt, 1.2f, { 0.35f, 1.f, 0.65f, 1.f });
            }
            prev = pt;
        }
        bool held = route.originHeld;
        for (int f : route.flips) {
            held = !held;
            int k = f - route.posBase - 1;
            if (f >= tickCount && k >= 0 && k < static_cast<int>(route.pos.size())) {
                d->drawDot(route.pos[k], held ? 3.f : 2.f, held ? ccColor4F { 0.3f, 0.95f, 1.f, 1.f } : ccColor4F { 1.f, 0.45f, 0.75f, 1.f });
            }
        }
        if (route.stuck) {
            int k = route.stuckAt - route.posBase - 1;
            if (k >= 0 && k < static_cast<int>(route.pos.size())) {
                d->drawDot(route.pos[k], 5.f, { 1.f, 0.2f, 0.2f, 1.f });
            }
        }
    }

    // v3.0 test 46: the route starts again from where you are, keeping its inputs still to come
    // (they're checked again from here). Used by the re-check and, since test 46, by a drift (test
    // 45 threw the whole route away on a 5-unit drift, right before a spike, with no time to plan)
    bool routeRebase(PlayLayer* layer, PlayerObject* p1) {
            std::vector<int> future;
            std::vector<RouteGroup> futureGroups;
            for (int f : route.flips) {
                if (f >= tickCount) {
                    future.push_back(f);
                }
            }
            for (auto const& g : route.groups) {
                if (g.first >= tickCount) {
                    futureGroups.push_back(g);
                }
            }
            // the button just before this tick's own inputs, as the old route had it
            bool heldNow = routeHeldAt(tickCount);
            routeStart(layer, p1);
            if (!route.active) {
                return false;
            }
            route.originHeld = heldNow;
            route.cps.front().held = heldNow;
            route.flips = future;
            route.groups = futureGroups;
        return true;
    }
