//@ hooks/ring_object.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
//@ 
//@ 
//@ 
// v2.57: an orb the ghost uses while trying ticks played its "used" animation on the real
// level (seen on dash orbs before you reached them). The state is restored afterwards
// anyway, so ghosts skip the visuals.
class $modify(BufRingObject, RingObject) {
    static void onModify(auto& self) {
        (void)self.setHookPriority("RingObject::powerOnObject", HOOK_PRIORITY);
        (void)self.setHookPriority("RingObject::spawnCircle", HOOK_PRIORITY);
    }

    void powerOnObject(int state) {
        if (buf.simulating) {
            return;
        }
        RingObject::powerOnObject(state);
    }

    void spawnCircle() {
        if (buf.simulating) {
            return;
        }
        RingObject::spawnCircle();
    }

    // v2.78.3: the activation call itself too (orbs and dash orbs still animated early)
    // v2.99.3: not on iOS/macOS. There it is a 12-byte stub that jumps into powerOnObject
    // (hooked above anyway), too short for an arm64 hook, which overwrote powerOnObject's start.
#if !defined(GEODE_IS_IOS) && !defined(GEODE_IS_MACOS)
    void triggerActivated(float xPosition) {
        if (buf.simulating) {
            return;
        }
        RingObject::triggerActivated(xPosition);
    }
#endif
};
