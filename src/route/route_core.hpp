//@ route/route_core.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    void beamReset();
    void pfReset();
    void routeReset() {
        for (auto& c : route.cps) {
            snapFree.push_back(c.snap);
        }
        route = {};
        beamReset();
        pfReset();
    }

    void routeKeepCps(int upTo) {
        // the saved states after `upTo` belong to a route that changed there
        while (route.cps.size() > 1 && route.cps.back().tick > upTo) {
            snapFree.push_back(route.cps.back().snap);
            route.cps.pop_back();
        }
        int keep = std::max(0, route.cps.back().tick - route.posBase);
        if (static_cast<int>(route.pos.size()) > keep) {
            route.pos.resize(keep);
        }
        route.end = route.cps.back().tick;
        route.changed = true;
    }

    std::vector<CCPoint> routeRec;
    std::vector<RoutePoint> routeNewCps;
    int routeSimStart = 0;

    void routeCpSink(int rel, PlayerObject* ghost, bool held) {
        if (route.cps.size() + routeNewCps.size() >= static_cast<size_t>(SNAP_MAX) - 2) {
            return;
        }
        auto* s = takeSnap();
        if (!s) {
            return;
        }
        copyState(s, ghost);
        routeNewCps.push_back({ routeSimStart + rel, s, held, buf.gBounds, buf.gBoundLo, buf.gBoundHi });
    }

    int routeCpEvery() {
        return st(48);
    }

    // the route's hidden player from saved state `from`, with `flips`, for `length` ticks: how many it lives
    int routeSim(PlayLayer* layer, PlayerObject* p1, RoutePoint const& from, std::vector<int> const& flips, int length, bool record) {
        std::vector<int> rel;
        for (int t : flips) {
            if (t >= from.tick) {
                rel.push_back(t - from.tick);
            }
        }
        float savedStep = buf.firstStep;
        bool orbClick = buf.orbClick, orbAfter = buf.orbAfterFirst, portals = buf.portalPredict;
        buf.firstStep = 1.f;
        buf.fullHorizon = true;
        buf.simFrom = from.snap;
        buf.simFromHeld = from.held;
        buf.simFromBounds = from.hasBounds;
        buf.simFromLo = from.lo;
        buf.simFromHi = from.hi;
        buf.simOffset = from.tick - tickCount;
        buf.crossPos = &route.pos;
        buf.crossBase = route.posBase - tickCount;
        buf.offscreenFill = true;
        buf.orbClick = false;
        buf.orbAfterFirst = false;
        buf.portalPredict = true;
        buf.orbTouches.clear();
        buf.orbTouchLast.clear();
        buf.plan = &rel;
        buf.skipPress = true;
        routeRec.clear();
        routeNewCps.clear();
        routeSimStart = from.tick;
        if (record) {
            buf.recordPath = &routeRec;
            if (from.snap) {
                buf.cpEvery = pfActive ? pfStepLen : routeCpEvery();
                buf.cpSink = pfActive ? &pfSink : &routeCpSink;
            }
        }
        buf.yBand = from.snap && (from.snap->m_isDart || from.snap->m_isShip) && from.tick > route.bandOffUntil;
        if (buf.yBand) {
            buf.yBandCenter = from.snap->getPositionY();
            buf.yBandHalf = 75.f;
        }
        SimEnd end;
        auto simStart = std::chrono::steady_clock::now();
        int lived = simulate(layer, p1, nullptr, false, 0, true, length, -1, length, &end);
        routeSimMs += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - simStart).count();
        buf.yBand = false;
        ++routeSimCount;
        buf.recordPath = nullptr;
        buf.cpEvery = 0;
        buf.cpSink = nullptr;
        buf.skipPress = false;
        buf.plan = nullptr;
        buf.simFrom = nullptr;
        buf.crossPos = nullptr;
        buf.offscreenFill = false;
        buf.orbClick = orbClick;
        buf.orbAfterFirst = orbAfter;
        buf.portalPredict = portals;
        buf.fullHorizon = false;
        buf.firstStep = savedStep;
        ++autoplay.sims;
        return lived;
    }

    bool routeHeldAt(int t) {
        // the button just before tick t's own flips
        bool held = route.originHeld;
        for (int f : route.flips) {
            if (f >= t) {
                break;
            }
            held = !held;
        }
        return held;
    }

    RoutePoint const& routeCpAtOrBefore(int t) {
        size_t i = route.cps.size();
        while (i > 1 && route.cps[i - 1].tick > t) {
            --i;
        }
        return route.cps[i - 1];
    }

    void routeStart(PlayLayer* layer, PlayerObject* p1) {
        routeReset();
        auto* s = takeSnap();
        if (!s) {
            return;
        }
        copyState(s, p1);
        route.active = true;
        route.origin = tickCount;
        route.originHeld = buf.appliedInput;
        RoutePoint cp0 { tickCount, s, buf.appliedInput };
        realBounds(layer, p1, cp0.hasBounds, cp0.lo, cp0.hi);
        route.cps.push_back(cp0);
        route.posBase = tickCount;
        route.end = tickCount;
        route.frontier = tickCount;
        route.changed = true;
    }
