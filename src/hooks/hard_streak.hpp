//@ hooks/hard_streak.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
//@ 
//@ 
class $modify(BufHardStreak, HardStreak) {
    static void onModify(auto& self) {
        (void)self.setHookPriority("HardStreak::addPoint", HOOK_PRIORITY);
    }

    void addPoint(CCPoint point) {
        if (buf.simulating) {
            return;
        }
        HardStreak::addPoint(point);
    }
};
