//@ scoring/next_click.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // v2.83 two clicks ahead: a run can crash only because the ghost never clicks again (the
    // next obstacle needs your next click). One more tap is tried 8 / 16 / 26 ticks before the
    // crash, but never sooner than 24 ticks after this click (a real second click, not a
    // double tap). If it then lives 40 ticks past the old crash (or the whole look-ahead), the
    // crash wasn't this click's. Returns when that tap is (ticks after the click), or -1.
    constexpr int NEXT_CLICK_MARGINS[] = { 8, 16, 26 };
    constexpr int NEXT_CLICK_MIN = 24;

    int nextClickAt(PlayLayer* layer, PlayerObject* p1, PlayerObject* p2, bool dual, int delay, bool down, int horizon, int releaseAfter,
        int ret) {
        if (!buf.nextClick || ret >= horizon || ret < st(NEXT_CLICK_MIN + 8)) {
            return -1;
        }
        int length = std::min(horizon, ret + st(40));
        for (int margin : NEXT_CLICK_MARGINS) {
            int at = ret - st(margin);
            if (at < st(NEXT_CLICK_MIN) || clickOverTime()) {
                break;
            }
            buf.retapAt = at;
            SimEnd e;
            int r = simulate(layer, p1, p2, dual, delay, down, length, releaseAfter, length, &e);
            buf.retapAt = -1;
            if (r >= length) {
                return at;
            }
        }
        return -1;
    }
