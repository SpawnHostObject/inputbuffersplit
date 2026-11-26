//@ core/tracking_types.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // v2.4 moving-object guard: last known position of each object the ghost collided with.
    struct Watched {
        double x;
        double y;
        int movedTick;
        double vx = 0.0;  // v2.27: movement per physics step while it's moving
        double vy = 0.0;
        double pvx = 0.0;  // v2.68: the step before (curved paths: how the direction turns)
        double pvy = 0.0;
        double rot = 0.0;  // v2.68: rotation, and how much it turns per step
        double vrot = 0.0;
    };
    constexpr int MOVER_TICKS = 120;
    constexpr size_t WATCH_LIMIT = 4096;
    std::unordered_map<GameObject*, Watched> watched;

    // v2.4 drift check: the recorded ghost run for your last input and how far you strayed.
    struct Drift {
        std::vector<CCPoint> path;
        bool active = false;
        int index = -1;
        int compared = 0;
        float maxErr = 0.f;
        int maxAt = 0;
    } drift;
