//@ scoring/robot_score.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // v2.7 wave search, same rules as the cube: the smallest delay that is safe (or
    // clearly better than now) wins, so turns are only held back as long as needed.
    // v2.12 robot scoring. A robot press is a jump whose height you set by when you let go,
    // so "pressing now crashes" only if NO release time saves it. Your usual hold is tried
    // first, then short to long holds. (Before, only your usual hold was tested, so a press
    // that needed a shorter or longer hold looked like a death and got delayed for nothing.)
    // v2.82: 8 hold lengths up to a full-height jump (was 4, up to 45 ticks), tried nearest
    // to your usual hold first, so the jump height a gap needs is found more often.
    constexpr int ROBOT_HOLDS[] = { 4, 9, 14, 20, 27, 36, 48, 64 };
    int robotUsedHold = -1;   // hold length that made the last robot run survive

    // v2.73: quick pass of a robot / UFO / spider search: only the plain run per delay (your
    // usual robot hold, no extra flap / flip). Those modes tried up to 5x more runs per
    // delay, so the search ran out of time after 2-10 delays and missed saves further out.
    bool quickScore = false;

    int robotScore(PlayLayer* layer, PlayerObject* p1, PlayerObject* p2, bool dual, int delay, int horizon, int target, SimEnd* end) {
        int usual = robotHold.pick(st(20), 50);
        int best = simulateOrbs(layer, p1, p2, dual, delay, true, horizon, usual, target, end);
        robotUsedHold = usual;
        if (best >= target || *end == SimEnd::StopSearch || quickScore) {
            return best;
        }
        int holds[std::size(ROBOT_HOLDS)];
        std::copy(std::begin(ROBOT_HOLDS), std::end(ROBOT_HOLDS), holds);
        std::stable_sort(std::begin(holds), std::end(holds), [&](int a, int b) { return std::abs(st(a) - usual) < std::abs(st(b) - usual); });
        for (int hold : holds) {
            if (std::abs(st(hold) - usual) < st(3)) {
                continue;
            }
            if (clickOverTime()) {
                break;
            }
            SimEnd e;
            int ret = simulateOrbs(layer, p1, p2, dual, delay, true, horizon, st(hold), target, &e);
            if (ret > best) {
                best = ret;
                robotUsedHold = st(hold);
            }
            if (best >= target) {
                break;
            }
        }
        return best;
    }
