//@ decision/simulate_orbs.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    int simulateOrbs(PlayLayer* layer, PlayerObject* p1, PlayerObject* p2, bool dual, int delay, bool down, int horizon, int releaseAfter,
        int stopAt, SimEnd* end) {
        buf.orbClick = buf.assumeOrbs;
        buf.touchedOrb = false;
        int first = simulate(layer, p1, p2, dual, delay, down, horizon, releaseAfter, stopAt, end);
        if (!buf.tryBothOrbs || !buf.smart || !buf.touchedOrb || first >= stopAt || clickOverTime()) {
            return first;
        }
        SimEnd firstEnd = *end;
        buf.orbClick = !buf.assumeOrbs;
        int second = simulate(layer, p1, p2, dual, delay, down, horizon, releaseAfter, stopAt, end);
        buf.orbClick = buf.assumeOrbs;
        if (firstEnd != SimEnd::StopSearch) {
            *end = SimEnd::Ok;
        }
        int best = std::max(first, second);
        // v2.26: some orbs only work when clicked at a specific moment inside them (levels
        // like Orbit). The ghost's orb clicks are also tried 3 and 6 ticks into the orb.
        if (buf.orbTiming && best < stopAt && !clickOverTime()) {
            for (int late : { 3, 6 }) {
                buf.orbClick = true;
                buf.orbClickDelay = st(late);
                SimEnd lateEnd;
                int ret = simulate(layer, p1, p2, dual, delay, down, horizon, releaseAfter, stopAt, &lateEnd);
                buf.orbClickDelay = 0;
                buf.orbClick = buf.assumeOrbs;
                best = std::max(best, ret);
                if (best >= stopAt || clickOverTime()) {
                    break;
                }
            }
        }
        return best;
    }
