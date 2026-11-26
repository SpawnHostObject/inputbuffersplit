//@ autoplay/short_planner.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // v3.0 test 5: autoplay plans a whole route (every press and release for the next half second)
    // instead of deciding one click at a time, and sticks to it. One-click-at-a-time chose any
    // corridor that survived a moment, so in levels with several paths it went into dead ends
    // and flipped the button almost every tick.
    struct PlanCtx {
        PlayLayer* layer;
        PlayerObject* p1;
        PlayerObject* p2;
        bool dual;
        int horizon;
        bool held0;     // the button now
        bool holdMode;  // wave / ship: holding is a real choice; other modes default to let go
        int at = 0;     // attempt tick now
        std::set<int> const* banned = nullptr;  // attempt ticks where a flip already led to a crash
    };

    // the ghost runs the plan; orbs are only clicked by the plan itself (v3.0 test 8: "assume
    // orbs" made the ghost click every orb on its own, so plans counted on orb clicks the real
    // autoplay never made)
    int simulatePlan(PlanCtx const& c, std::vector<int> const& rel) {
        SimEnd end;
        bool orbClick = buf.orbClick, orbAfter = buf.orbAfterFirst, portals = buf.portalPredict;
        buf.orbClick = false;
        buf.orbAfterFirst = false;
        buf.portalPredict = true;  // v3.0 test 14: stopping at a portal looked "safe" to the planner
        buf.orbTouches.clear();
        buf.plan = &rel;
        buf.skipPress = true;
        int survived = simulate(c.layer, c.p1, c.p2, c.dual, 0, true, c.horizon, -1, c.horizon, &end);
        buf.skipPress = false;
        buf.plan = nullptr;
        buf.orbClick = orbClick;
        buf.orbAfterFirst = orbAfter;
        buf.portalPredict = portals;
        ++autoplay.sims;
        return survived;
    }

    // depth-limited search: flips go in right before the ghost would die, latest first (late and
    // few flips is how a player plays); returns ticks survived, `best` gets the longest plan seen
    bool planOutOfTime() {
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - autoplay.searchStart).count() > autoplay.searchCap;
    }

    int planSearch(PlanCtx const& c, std::vector<int>& rel, int depth, int budget, std::vector<int>& best, int& bestLen) {
        if (planOutOfTime()) {
            autoplay.sims = budget;  // stop here, carry on next frame
            return -1;
        }
        int survived = simulatePlan(c, rel);
        if (survived > bestLen || (survived == bestLen && rel.size() < best.size())) {
            bestLen = survived;
            best = rel;
        }
        if (survived >= c.horizon || depth == 0 || autoplay.sims >= budget) {
            return survived;
        }
        int last = rel.empty() ? -1 : rel.back();
        // orbs touched before the crash: try a tap right on them first (a press while touching it)
        std::vector<int> orbTicks;
        for (auto const& [r, t] : buf.orbTouches) {
            if (t > last && t <= survived && orbTicks.size() < 2) {
                orbTicks.push_back(t);
            }
        }
        for (int ot : orbTicks) {
            if (c.banned && c.banned->count(c.at + ot)) {
                continue;
            }
            bool heldThen = c.held0 != (rel.size() % 2 == 1);
            size_t before = rel.size();
            if (heldThen) {
                rel.push_back(ot);  // let go, then press again on the orb
                rel.push_back(ot + 1);
            }
            else {
                rel.push_back(ot);
            }
            if (!c.holdMode) {
                rel.push_back(rel.back() + st(4));  // and let go again: a tap
            }
            int r = planSearch(c, rel, depth - 1, budget, best, bestLen);
            rel.resize(before);
            if (r >= c.horizon || autoplay.sims >= budget) {
                return r;
            }
        }
        // v3.0 test 11: how long before the crash to flip. A cube jump lasts ~100 ticks, so
        // clearing a spike often means pressing 30-60 ticks before reaching it (test 10 only tried
        // up to 30, so it never found that jump). Flying modes need finer, closer steps.
        static constexpr int GROUND_BACKS[] = { 2, 5, 9, 14, 20, 27, 35, 45, 56, 70, 85, 100 };
        static constexpr int FLY_BACKS[] = { 1, 2, 4, 6, 9, 13, 18, 24 };
        std::vector<int> backs;
        if (c.holdMode) {
            backs.assign(std::begin(FLY_BACKS), std::end(FLY_BACKS));
        }
        else {
            backs.assign(std::begin(GROUND_BACKS), std::end(GROUND_BACKS));
        }
        // v3.0 test 19: then earlier and earlier, all the way back to the last flip. The way past a
        // spike can be a jump long before it (onto another path); test 18 showed the ghost hitting
        // the same spike as you, with no press ever tried more than 100 ticks before it.
        {
            int stepBack = c.holdMode ? 6 : 12;
            for (int b = backs.back() + stepBack; st(b) <= survived - last; b += stepBack) {
                backs.push_back(b);
            }
        }
        for (int back : backs) {
            int t = survived - st(back);
            if (t <= last) {
                if (back == backs.front()) {
                    t = last + 1;  // at least try the very next tick
                }
                else {
                    break;
                }
            }
            if (t < 0 || t > survived || (c.banned && c.banned->count(c.at + t))) {
                continue;
            }
            // v3.0 test 16: outside wave / ship a press is tried three ways: a short tap, a longer
            // tap, and holding (let go later by the next step). Test 9-15 only tried a 3-tick tap,
            // which does nothing when pressed just before landing (the game jumps on landing only
            // while you still hold) and can't chain a jump right after landing.
            bool heldAt = c.held0 != (rel.size() % 2 == 1);
            int variants[] = { st(3), st(10), -1 };
            int nVariants = (!c.holdMode && !heldAt) ? 3 : 1;
            int r = -1;
            for (int v = 0; v < nVariants; ++v) {
                size_t before = rel.size();
                rel.push_back(t);
                if (nVariants > 1 && variants[v] > 0) {
                    rel.push_back(t + variants[v]);
                }
                r = planSearch(c, rel, depth - 1, budget, best, bestLen);
                rel.resize(before);
                if (r >= c.horizon || autoplay.sims >= budget) {
                    return r;
                }
            }
            if (t == last + 1) {
                break;
            }
        }
        return survived;
    }

    // v3.0 test 20: the plans one step on from `plan` (absolute flip ticks), which lasted `survived`
    // ticks from now and touched `orbs`: a tap on an orb it touched, then a press or release near
    // the crash and further and further back (a short tap, a long tap or a hold outside wave / ship)
    void enqueueChildren(PlanCtx const& c, std::vector<int> const& plan, int survived, std::vector<std::pair<void*, int>> const& orbs) {
        int lastRel = plan.empty() ? -1 : plan.back() - tickCount;
        auto push = [&](std::vector<int> child) {
            if (autoplay.seen.insert(child).second) {
                autoplay.queue.push_back({ survived, std::move(child) });
                std::push_heap(autoplay.queue.begin(), autoplay.queue.end());
            }
        };
        auto banned = [&](int rel) {
            return c.banned && c.banned->count(c.at + rel);
        };
        bool heldAt = c.held0 != (plan.size() % 2 == 1);
        for (auto const& [r, t] : orbs) {
            if (t > lastRel && t <= survived && !banned(t)) {
                std::vector<int> child = plan;
                if (heldAt) {
                    child.push_back(tickCount + t);
                    child.push_back(tickCount + t + 1);
                }
                else {
                    child.push_back(tickCount + t);
                }
                if (!c.holdMode) {
                    child.push_back(child.back() + st(4));
                }
                push(std::move(child));
            }
        }
        // v3.0 test 21: every 3 ticks (2 in wave / ship) from the crash all the way back. Test 20 had
        // gaps of 10-15 ticks, so a jump that only works in a window of a few ticks was stepped over
        std::vector<int> backs;
        int stepBack = c.holdMode ? 2 : 3;
        for (int b = 1; st(b) <= survived - lastRel; b += stepBack) {
            backs.push_back(b);
        }
        for (int back : backs) {
            int t = std::max(survived - st(back), lastRel + 1);
            if (t < 0 || t > survived || banned(t)) {
                continue;
            }
            if (!c.holdMode && !heldAt) {
                for (int len : { st(3), st(10), -1 }) {
                    std::vector<int> child = plan;
                    child.push_back(tickCount + t);
                    if (len > 0) {
                        child.push_back(tickCount + t + len);
                    }
                    push(std::move(child));
                }
            }
            else {
                std::vector<int> child = plan;
                child.push_back(tickCount + t);
                push(std::move(child));
            }
            if (t == lastRel + 1) {
                break;
            }
        }
        if (autoplay.queue.size() > 20000) {
            std::sort_heap(autoplay.queue.begin(), autoplay.queue.end());
            autoplay.queue.erase(autoplay.queue.begin(), autoplay.queue.end() - 10000);
            std::make_heap(autoplay.queue.begin(), autoplay.queue.end());
        }
    }

    // v3.0 test 21: a plan got further than the one it came from: try its last press a tick or two
    // earlier and later too (exact timings), ahead of everything else in the queue
    void enqueueNeighbours(PlanCtx const& c, std::vector<int> const& plan, int score, int pressIndex) {
        for (int d : { -2, -1, 1, 2 }) {
            std::vector<int> child = plan;
            for (size_t i = pressIndex; i < child.size(); ++i) {
                child[i] += st(d);
            }
            int prev = pressIndex > 0 ? child[pressIndex - 1] : tickCount - 1;
            if (child[pressIndex] <= prev || child[pressIndex] < tickCount) {
                continue;
            }
            if (c.banned && c.banned->count(c.at + child[pressIndex] - tickCount)) {
                continue;  // test 26: a press that already failed there
            }
            if (autoplay.seen.insert(child).second) {
                autoplay.queue.push_back({ score, std::move(child) });
                std::push_heap(autoplay.queue.begin(), autoplay.queue.end());
            }
        }
    }

    void clearQueue() {
        autoplay.queue.clear();
        autoplay.seen.clear();
        autoplay.queueBase = -1;
    }
