//@ route/route_tick.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    bool routeTick(PlayLayer* layer, PlayerObject* p1, int at) {
        if (snapAll.empty() || snapOwner != layer) {
            return false;
        }
        // test 92: a debug tool: only with Drift Check on, and every 8th tick (it ran every tick)
        bool sampleStep = buf.driftCheck && tickCount % 8 == 0;
        if (sampleStep) {
            stepCheck(layer, p1);
        }
        stepChk.tick = -1;
        // are you where the route said you'd be?
        if (route.active && tickCount - 1 >= route.origin) {
            int k = tickCount - 1 - route.posBase;
            if (k >= 0 && k < static_cast<int>(route.pos.size())) {
                auto want = route.pos[k];
                auto now = p1->getPosition();
                // v3.0 test 54: only a big difference (10 units) replans at once; a small one is left to
                // the re-check from you, which replans only if it finds a death (a 5-unit ship
                // difference replanned 0.3 s before a wave portal: Aquamarine 69%)
                if (!pf.active && (std::abs(want.x - now.x) > 10.f || std::abs(want.y - now.y) > 10.f)) {  // test 58: not while the pathfinder gives you its inputs
                    if (routeDrifts < 12) {
                        ++routeDrifts;
                        say("route drift at tick {} ({}): you're at {:.0f},{:.0f}, the route said {:.0f},{:.0f}, planning again from you", at,
                            MODE_NAMES[modeIndex(p1, nullptr)], now.x, now.y, want.x, want.y);
                    }
                    if (!routeRebase(layer, p1)) {
                        return false;
                    }
                }
            }
        }
        // a stuck route you got well past (the short planner found a way): a new one from you
        if (route.active && route.stuck && tickCount > route.stuckAt + st(60)) {
            routeReset();
        }
        if (!route.active) {
            routeStart(layer, p1);
            if (!route.active) {
                return false;
            }
        }
        // v3.0 test 30: what moves is only known well once you're close (the objects have been seen
        // moving). Every 0.05 s the next 1.25 s of the route are played again from you, with the
        // objects as they are now; if that dies sooner than the route said, the route is planned
        // again from you, keeping its inputs still to come (they're checked again from there)
        // v3.0 test 49: every 6 ticks (was 12), from where you really are, with the path compared too
        // v3.0 test 61: also while it looks for a way past a stuck spot further on, or the pathfinder
        // runs (tests 27-60 stopped checking then, so an object that moved into the path you were
        // following wasn't seen and you flew into it). Then only a death sooner than the one it
        // already knows about counts, and the path isn't re-anchored.
        bool busy = route.sprinting || pf.active;
        int checkEnd = route.end;
        if (pf.active) {
            checkEnd = pf.commits > 0 ? pf.start : std::min(route.end, pf.death);
        }
        // test 92: 20 times a second (was 40), 10 when frames are slow
        int recheckEvery = fpsScale < 0.6 ? st(24) : st(12);
        if (route.active && tickCount - route.lastRecheck >= recheckEvery && checkEnd - tickCount > st(6)) {
            route.lastRecheck = tickCount;
            RoutePoint here { tickCount, nullptr, buf.appliedInput };
            int length = std::min(checkEnd, tickCount + (fpsScale < 0.6 ? st(480) : st(720))) - tickCount;  // test 63: 3 s ahead (test 92: 2 s while frames are slow)
            int lived = routeSim(layer, p1, here, route.flips, length, true);
            // the route's path off from where you'll really go (more than 3 units, while both live):
            // the route is put back onto you, keeping its inputs
            // v3.0 test 56: the route was planned far ahead, where the ghost sees some parts slightly
            // differently (Aquamarine's ship parts: 10 units off at the same spots every time, also
            // without Click Between Frames). Seen from near you it's right, so when the next second
            // seen from you leaves the route by more than 4 units, the route is put back onto you
            // now (~1 s before the spot), keeping its inputs; at most every 0.25 s
            bool off = false;
            CCPoint offAt {};
            float offBy = 0.f;
            for (int k = 0; k < static_cast<int>(routeRec.size()) && k < lived; k += 2) {
                int i = tickCount + k - route.posBase;
                if (i < 0 || i >= static_cast<int>(route.pos.size())) {
                    break;
                }
                float d = std::max(std::abs(routeRec[k].x - route.pos[i].x), std::abs(routeRec[k].y - route.pos[i].y));
                if (d > 4.f) {
                    off = true;
                    offAt = route.pos[i];
                    offBy = d;
                    break;
                }
            }
            if (off && !busy && lived >= length && tickCount - route.lastAnchor >= st(60)) {
                route.lastAnchor = tickCount;
                if (routeRechecks < 16) {
                    ++routeRechecks;
                    int slopes = 0;
                    int b = static_cast<int>(std::floor(offAt.x / 64.f));
                    for (int kk = b - 2; kk <= b + 2; ++kk) {
                        auto it = buf.objIndex.find(kk);
                        if (it == buf.objIndex.end()) {
                            continue;
                        }
                        for (auto* o : it->second) {
                            if (o->m_objectType == GameObjectType::Slope && std::abs(o->m_positionX - offAt.x) < 90.f &&
                                std::abs(o->m_positionY - offAt.y) < 90.f) {
                                ++slopes;
                            }
                        }
                    }
                    say("route re-anchored on you at tick {}: {:.0f} units off ahead at x {:.0f} y {:.0f} ({} slopes near it), inputs kept", at,
                        offBy, offAt.x, offAt.y, slopes);
                }
                if (!routeRebase(layer, p1)) {
                    return false;
                }
                route.lastRecheck = tickCount;
            }
            // test 33: one re-check isn't enough unless the death is close (guesses about moving
            // objects wobble, and every replan changed the path you see)
            route.recheckFails = lived < length ? route.recheckFails + 1 : 0;
            if (lived < length && (lived < st(90) || route.recheckFails >= 2 || (busy && lived < length - st(6)))) {
                if (routeRechecks < 12) {
                    ++routeRechecks;
                    say("route re-check at tick {}: it now dies {} ticks ahead, not past {} (something moved): planned again from you", at,
                        lived, length);
                }
                if (!routeRebase(layer, p1)) {
                    return false;
                }
                route.lastRecheck = tickCount;
            }
        }
        // the route's saved states behind you aren't needed any more
        while (route.cps.size() > 1 && route.cps[1].tick <= tickCount) {
            snapFree.push_back(route.cps.front().snap);
            route.cps.erase(route.cps.begin());
        }

        // plan: a slice of the frame
        unsigned frame = CCDirector::get()->getTotalFrames();
        if (frame != autoplay.frame) {
            autoplay.frame = frame;
            autoplay.frameMs = 0.0;
        }
        double frameMs = std::max(1.0, CCDirector::get()->getDeltaTime() * 1000.0);
        frameGuard(frameMs);
        // v3.0 test 44: back to about half a frame (test 35's double time dropped the FPS)
#ifdef GEODE_IS_MOBILE
        double budget = std::clamp(frameMs * 0.5, 2.0, 5.0);
#else
        double budget = std::clamp(frameMs * 0.5, 2.0, 8.0);
#endif
        if (route.end - tickCount < st(60) || pf.active) {
            budget *= 1.5;  // the planned part is about to run out (test 58: or the pathfinder is racing you)
        }
        // test 92: never more than your Prediction Budget setting, and less while frames are slow
        budget = std::min(budget, std::max(2.0, buf.userBudget * 1.25)) * fpsScale;
        auto start = std::chrono::steady_clock::now();
        double left = budget - autoplay.frameMs;
        while (left > 0.2) {
            if (pf.active) {
                pfStep(layer, p1);
            }
            else if (beam.active) {
                beamStep(layer, p1);
            }
            else if (route.sprinting) {
                routeSprintStep(layer, p1);
            }
            else if (!routeExtend(layer, p1)) {
                break;
            }
            double used = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
            left = budget - autoplay.frameMs - used;
        }
        sweepGhostLayer();
        autoplay.frameMs += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

        if (tickCount >= route.end && route.stuck) {
            return false;  // stuck here: the short planner looks after this part
        }
        // (test 28: while the route is being redone from a saved state behind you, you keep its
        // inputs: test 27 let the short planner press here, which put you off the route, and the
        // route started over from nothing again and again)
        // the button as the route has it for this tick
        bool want = route.originHeld;
        for (int f : route.flips) {
            if (f > tickCount) {
                break;
            }
            want = !want;
        }
        if (stepChk.snap && !p1->m_isDead && buf.driftCheck && (tickCount + 1) % 8 == 0) {
            // test 64: your state now, played one tick by the ghost next time
            copyState(stepChk.snap, p1);
            stepChk.tick = tickCount;
            stepChk.held = buf.appliedInput;
            stepChk.want = want;
            realBounds(layer, p1, stepChk.hasB, stepChk.lo, stepChk.hi);
        }
        autoplaySet(layer, want);
        autoplay.plan.clear();
        autoplay.verifyAt = -1;
        autoplay.planSafeUntil = at + std::max(0, route.end - tickCount);
        if (!route.stuck) {
            autoplay.killer = "the route said this was safe";
        }
        routeDraw(p1);
        return true;
    }
