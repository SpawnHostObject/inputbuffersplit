//@ route/route_types.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // ===== v3.0 test 27: the route =====
    // A route is planned from where you are toward the end of the level (up to 10 s ahead), a bit
    // more every tick. Hidden-player states are saved along it every 0.2 s, so it grows from its
    // end (not from you every time). Where it dies, an input is tried one tick earlier at a time
    // (latest first), and the middle of the window that works is kept. Nothing works: the last
    // input is taken back and it looks before it (up to 8 times). You're checked against the
    // route every tick; off it, it plans again from where you are.
    struct RoutePoint {
        int tick;  // the state at the start of this tick (before its inputs)
        PlayerObject* snap;
        bool held;
        bool hasBounds = false;  // test 50: the corridor the ghost was in there
        float lo = 0.f, hi = 0.f;
    };
    struct RouteGroup {
        int first;
        int last;
    };
    struct Route {
        bool active = false;
        int origin = 0;
        bool originHeld = false;
        std::vector<int> flips;  // absolute ticks the button flips at, ascending
        std::vector<RouteGroup> groups;
        std::vector<RoutePoint> cps;
        std::vector<CCPoint> pos;  // pos[k]: where the route is after tick posBase + k
        int posBase = 0;
        int end = 0;  // the route is planned up to the start of this tick
        bool sprinting = false;
        int death = 0;
        int cand = 0;
        int lower = 0;
        int variant = 0;
        int windowHi = -1;
        int windowVariant = 0;
        int bestT = -1;  // test 29: the input that got furthest so far in this search
        int bestV = 0;
        int bestEnd = 0;
        int sprintTests = 0;  // test 41: what the search did, for the "stuck" report
        int orbTries = 0;
        int orbBest = 0;
        int lowest = INT_MAX;
        int orbT = -1;  // test 38: an input whose path reaches an orb the route hadn't touched
        int orbV = 0;
        int orbEnd = 0;
        int takebacks = 0;
        int frontier = 0;
        std::set<int> bannedFirst;
        bool stuck = false;
        int stuckAt = 0;
        int drawnAt = -1000;
        bool changed = false;
        bool fine = false;  // test 28: the search goes a few ticks at a time first, then tick by tick
        int candStart = 0;
        int lastRecheck = -1000;
        int lastAnchor = -100000;  // test 56
        int recheckFails = 0;  // test 33: re-checks in a row that died sooner than the route
        int beforeLimit = INT_MAX;  // test 32: after a take-back, the next search looks before it
        int bandOffUntil = INT_MIN;  // test 45: the band is off for tests starting before this tick
        bool afterTakeback = false;  // test 37: the next working timing is taken as it is (latest first)
        std::vector<int> orbFirst;  // test 35: clicks in the middle of the orbs passed before the death, tried first
        std::vector<std::pair<int, int>> orbRanges;  // test 39: when the route touches orbs (first, last tick)
        std::string why;
    };
    Route route;
    double routeSimMs = 0.0;  // test 28: what the route's tests cost (shown at a death)
    int routeSimCount = 0;
    int routeReports = 0;
    int routeDrifts = 0;
    int routeRechecks = 0;
    std::map<int, int> routeStuckSpots;
    void routeCountersReset() {
        routeStuckSpots.clear();
        buf.portalLogs = 0;
        routeReports = 0;
        routeDrifts = 0;
        routeRechecks = 0;
        routeSimMs = 0.0;
        routeSimCount = 0;
        buf.offscreenAdded = 0;
        buf.offscreenFallback = 0;
    }
