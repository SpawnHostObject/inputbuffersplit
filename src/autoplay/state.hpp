//@ autoplay/state.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    bool macroFixesSetting = true;

    // test 89: a bot's position record for one frame (player 1, and player 2 if it has one)
    struct MacroFix {
        int frame = 0;
        float x = 0.f, y = 0.f, r = 0.f;
        bool p2 = false;
        float x2 = 0.f, y2 = 0.f, r2 = 0.f;
    };

    struct Autoplay {
        bool active = false;
        PlayLayer* layer = nullptr;
        int inputs = 0;
        int userUntil = -1;   // Take Over: your last click; the bot waits until this tick
        bool takeOver = false;
        std::vector<int> plan;  // absolute ticks at which to flip the button
        int verifyAt = -1;      // next tick to check the plan still works
        int sims = 0;           // planner budget used for one search
        // v3.0 test 7: a time budget per frame (the "budget" setting), so planning never drops
        // the frame rate; what doesn't fit goes on next frame while the current plan is followed
        unsigned frame = 0;
        double frameMs = 0.0;
        std::chrono::steady_clock::time_point searchStart {};
        double searchCap = 0.0;
        // v3.0 test 8: it learns the level (normal mode). Each attempt's inputs are kept; after a
        // death the ones up to a bit before it are replayed exactly next attempt, and the search
        // only runs past them. Dying at the same spot again and again forgets further back.
        int attemptTick = 0;
        // v3.0 test 10: inputs are kept as states (tick, pressed) and every attempt starts let go,
        // so a replay is the same each time (flips replayed from a held start came out inverted)
        std::vector<std::pair<int, bool>> attemptFlips;
        std::vector<std::pair<int, bool>> known;
        int planSafeUntil = -1;  // attempt tick until which the current plan was expected to survive
        int knownUntil = 0;
        size_t replayIdx = 0;
        bool userTouched = false;
        int lastDeath = -1;
        int sameDeaths = 0;
        bool diedThisAttempt = false;  // noclip (Mega Hack) calls "death" every tick: count one
        int reports = 0;               // v3.0 test 15: search reports logged this attempt
        // v3.0 test 17: flips (attempt ticks) that led to a crash, never tried there again (kept
        // across attempts), and searches in a row that couldn't make the plan last longer
        std::set<int> banned;
        int stuck = 0;
        std::string killer;  // v3.0 test 18: what kills the ghost in the current plan
        // v3.0 test 20: plans still to try (absolute flip ticks, longest-lasting parent first),
        // kept from frame to frame so the search goes on where it stopped instead of trying the
        // same first few plans every frame
        struct QItem {
            int score;
            std::vector<int> plan;
            bool operator<(QItem const& o) const {
                return score != o.score ? score < o.score : plan.size() > o.plan.size();
            }
        };
        std::vector<QItem> queue;
        std::set<std::vector<int>> seen;
        int queueBase = -1;  // tick the queue was built at (cleared when a flip happens)
        // v3.0 test 24: the trail. Where you were every 4 ticks of this attempt, and of the part
        // it knows; drawn in the level, saved with the inputs so it's known from the first attempt
        std::vector<CCPoint> attemptTrail;
        std::vector<CCPoint> knownTrail;
        bool knowsWholeLevel = false;
        std::string fromMacro;  // test 67: the macro file autoplay replays (empty = its own memory)
        std::vector<std::pair<int, bool>> macro;  // test 70: its inputs, by step
        int macroUntil = 0;  // played up to here (less after it keeps dying at one spot)
        int macroEnd = 0;
        size_t macroIdx = 0;
        bool macroArmed = false;  // (test 70; test 71: not used)
        std::vector<float> macroX;  // test 71: where each input is (from the macro's position records), for start positions
        int attemptNo = 0;
        int macroSyncedFor = -1;
        bool macroMidStart = false;  // alpha.9: this attempt's macro began partway in (it was downloaded mid-attempt)
        float macroLastPx = -1.f;  // alpha.10: x a step ago (how far a step goes)
        int macroAheadTry = 0;  // alpha.11 (unused since alpha.13)
        std::vector<int> macroAheadReach;
        // alpha.13: spots a position-played macro keeps dying at (x) -> which nudge it's on there, and
        // the nudges done this attempt
        std::map<int, int> macroNudges;
        std::set<int> macroNudged;
        bool macroByX = false;
        bool macroForceX = false;
        // alpha.26: an xdBot macro is played exactly as xdBot plays it: its frame is the level time x TPS
        // + 1, its inputs and position records go in after the game's commands, once a frame
        bool macroXd = false;
        // alpha.27: physics steps per game frame while an xdBot macro plays (1: 240 FPS; 4: 60 FPS).
        // Some things (moving objects) only update on a frame's last step, so a macro made at 60 FPS
        // only lines up when the game runs at 60 FPS too (Green Bullet "60fps": a moving spike was
        // 36 units off with you exactly on the bot's path)
        int xdFrameSteps = 1;
        int xdFrameTry = 0;
        std::vector<int> xdFrameReach;
        size_t xdFixIdx = 0;
        int xdLastFrame = -1;  // v3.1.1: made at another TPS: played by x from its position records
        std::vector<float> macroXA, macroXB, macroX2;  // each list's x (A/B as in the file; X2: player 2's)
        size_t fixPtr = 0;
        bool macroLogged = false;
        std::vector<int> macroDeaths;
        std::vector<std::pair<int, int>> macroGaps;  // test 75: stretches autoplay plays itself, the macro the rest
        bool macroInGap = false;
        // test 90: the global brain: this attempt's button changes by the game's step (to share a finished
        // run), the level's fingerprint, and whether the macro is a shared run
        std::vector<std::pair<int, bool>> runFlips;
        std::vector<MacroFix> runFixes;  // v3.1.1: where the player was (every 3 steps and at each click), shared with the run
        std::string levelPrint;
        bool macroIsBrain = false;
        bool macroLdmMismatch = false;  // alpha.23: the macro's LDM isn't yours
        // v3.1.1: the brain run being played (its key on the site), and the ones that failed here
        std::string brainKey;
        std::vector<std::string> brainSkip;
        int brainSkipLevel = 0;
        bool brainReported = false;
        std::string macroUrl;  // v3.1.1: the Hyperbolus download the macro came from (empty: yours / the brain)
        std::vector<MacroFix> macroFixes;  // test 89: frames converted to steps
        int macroFixCount = 0;
        int macroFixSkipped = 0;  // v3.1.1: fixes left out (they'd have put you inside something)
        float macroFixMax = 0.f;
        int macroStepNow = 0;  // test 82: the macro's step as the game's step started (processCommands)
        // test 79: which step the macro's frame 0 is (bots differ by a step): tried in turn, the best kept
        int macroTry = 0;       // index into MACRO_OFFSETS while trying
        bool macroTuned = false;
        int macroAutoOffset = 0;
        std::array<int, 5> macroReach { -1, -1, -1, -1, -1 };  // (test 79; test 84: macroTrialReach)
        // test 84: 2-player levels: each player's clicks on their own (macro = player 1's, macroP2 = player 2's)
        bool macroTwoPlayer = false;
        bool macroSwap = false;  // the file's "player 1" / "player 2" the other way round (iCreate)
        std::vector<std::pair<int, bool>> macroA, macroB, macroP2;
        size_t macroIdx2 = 0;
        bool held2 = false;
        std::vector<std::pair<int, bool>> macroTrials;  // (shift, swapped) tried in turn
        std::vector<int> macroTrialReach;
        std::vector<int> deathTicks;  // where this session's attempts ended (learning backs off at repeats)
        // v3.0 test 51: the attempt that got furthest; the next one replays it to just before its end
        int bestReached = -1;
        std::vector<std::pair<int, bool>> bestFlips;
        std::vector<CCPoint> bestTrail;
        float startX = -2.f;  // the start position the learning is for
        std::string saveKey;
        size_t levelHash = 0;
        CCDrawNode* trailNode = nullptr;
        // v3.0 test 25: making the path. On start the level is played hidden, many times faster
        // than normal and with time to think at every jump, until it reaches the end; then the
        // whole trail is known and the run you see follows it from the start.
        bool generating = false;
        bool needRestart = false;
        int genAttempts = 0;
        float genBest = 0.f;
        std::chrono::steady_clock::time_point genStart {};
        std::chrono::steady_clock::time_point genEnded {};
        CCLayerColor* genCover = nullptr;
        CCLabelBMFont* genText = nullptr;
        CCLabelBMFont* tag = nullptr;
    };
