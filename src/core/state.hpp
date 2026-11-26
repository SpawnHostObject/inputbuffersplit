//@ core/state.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    struct Buffer {
        PlayerObject* ghost1 = nullptr;
        PlayerObject* ghost2 = nullptr;
        CCLayer* ghostLayer = nullptr;
        unsigned ghostLayerBase = 0;  // v2.24: children the ghost layer has right after the ghosts are made

        CCNode* panel = nullptr;
        CCLabelBMFont* panelTitle = nullptr;               // current stage
        std::vector<CCLabelBMFont*> panelLines;            // one label per log line (colored)
        bool panelOpen = false;  // closed by default, remembered for the session
        bool panelDirty = false; // v2.71: new log lines, the panel is redrawn once on the next step
        bool perfMode = false;   // v2.71: performance mode
        double userBudget = 6.0; // v2.75: the budget from the settings (budgetMs is this click's)
        bool autoBudget = true;  // v2.75: budget follows the frame rate
        std::unordered_set<GameObject*> simDisabled;  // v2.75: objects a toggle orb switched off in this prediction
        std::deque<std::string> logLines;

        bool simulating = false;
        bool injecting = false;
        bool ghostDied = false;
        bool ghostHitPortal = false;
        bool portalPredict = true;    // v2.79: the ghost takes mode portals instead of stopping there
        bool ghostSwitched = false;   // v2.79: the ghost changed mode through a portal in this run
        bool lastRunSwitched = false; // v2.83: the last finished run went through a mode portal
        bool ghostSolo = false;       // v2.84: a dual ghost went through a solo portal (only P1 goes on)
        bool simSecond = false;       // v2.85: ghost 2 is being simulated (dual from the start, or after a dual portal)
        bool noHitch = true;          // v2.85: a search that ran out of time goes on next tick
        std::vector<GameObject*> simEnabled;  // v2.85: switched-off objects a toggle switched on in this prediction
        bool nextClick = true;        // v2.83: plan two clicks ahead
        bool realInput = false;
        bool appliedInput = false;
        bool isPlayer1 = true;

        bool enabled = true;
        int stage = 0;
        int ticksAll = 2;
        int ticksWave = 2;

        std::deque<QueuedInput> queue;
        int shift = 0;  // delay applied to inputs while the queue is active

        float tickLength = TICK;
        int sinceInput = 999;
        int rapid = 0;

        bool smart = true;
        bool console = false;
        double budgetMs = 6.0;
        bool practiceOnly = true;    // v2.1: only buffer in practice mode
        int groundHorizon = 130;     // v2.1: how far ahead cube/ball/spider predictions look
        int releaseShift = 0;        // v2.1: delay of the last delivered press, reused for its release
        int waveHorizon = 65;        // v2.2: how far ahead wave predictions look (mini = 15 less)
        bool assumeOrbs = true;      // v2.2: the ghost clicks every orb it touches ("calculation mode")
        bool held1 = false, held2 = false;   // button state the ghosts are meant to be in
        bool auto1 = false, auto2 = false;   // an orb auto-click is currently held on this ghost
        bool orbTiming = true;       // v2.26: orb clicks at specific timings inside the orb
        int orbClickDelay = 0;       // v2.26: ghost orb auto-clicks wait this many ticks into the orb
        int autoAt1 = -1, autoAt2 = -1;      // v2.26: tick a delayed orb auto-click fires (-1 none, -2 done)
        bool orbWasTiming = false;   // v2.26: the last orb save re-timed a press that was already in the orb

        bool tryBothOrbs = true;     // v2.4: if a run touches an orb and dies, retry with the other orb choice
        bool orbClick = true;        // v2.4: orb choice of the current run
        bool touchedOrb = false;     // v2.4: the current run touched an orb
        bool fullHorizon = false;
        std::vector<int> const* plan = nullptr;  // v3.0 autoplay: ticks (from now) at which the ghost flips its button
        std::vector<std::pair<void*, int>> orbTouches;
        std::vector<int> orbTouchLast;  // v3.0 test 35: the last tick each of those orbs was still touched  // v3.0 test 8: orbs the planning ghost touched, and when    // v3.0 autoplay: only surviving the whole look-ahead counts (no "landed = safe")
        bool skipPress = false;      // v2.4: run without pressing anything (auto-release hold check)
        bool avoidMovers = true;     // v2.4: don't buffer when the ghost's path has moving objects
        bool sawMover = false;       // v2.4: a run this decision touched a moving object
        bool trackMovers = false;    // v2.5.1: watch objects only in this run (not in every search run)
        bool autoRelease = false;    // v2.4: wave auto-release (practice mode only)
        int autoWindow = 6;          // v2.4: hold must die within this many ticks to auto-release
        bool autoReleased = false;   // v2.4: we released for you; your real release is swallowed
        bool autoClick = false;      // v2.80: click for you at the last moment when you didn't
        bool autoClicked = false;    // v2.80: we clicked for you; your own late click is swallowed
        int autoClickTick = 0;       // v2.80: when that was
        PlayLayer* owner = nullptr;  // v2.86: the level the mod has set up (null while one loads)
        int spamGap = 16;            // v2.5.2: inputs closer than this (ticks) count toward wave spam
        int spamStreak = 3;          // v2.58: this many fast inputs in a row = spam (was fixed at 3)
        bool waveMargin = true;      // v2.10: center wave saves in the safe window (1 tick of room)
        bool waveV46 = false;        // v2.7: use the old v4.6 wave rules instead of the cube-style ones
        bool orbAlign = true;        // v2.6: hold an early press back until the ghost is inside the orb
        bool orbAlignGround = false; // v2.11.1: also align presses made on the ground (jumps)
        bool ignoreTouching = true;  // v2.31: hazards you touch right now without dying can't kill the ghost
        bool haveRealRect = false;
        bool haveStartRects = false;  // v2.78.1: realRect1/2 hold where you are as this prediction starts (portal check)
        CCRect realRect1, realRect2;  // your hitboxes when the prediction starts
        std::unordered_set<GameObject*> harmless;  // hazards on you at the start of this prediction
        int baseEscape = -1;         // v2.36: press-now run only survives if you react by this tick
        int baseCrash = -1;          // v2.36: ...otherwise it crashes here
        bool lowLatency = false;     // v2.35: never chain delays, small budget and max delay
        bool explain = false;        // v2.33: log why an input was passed through untouched
        bool deathLog = false;       // v2.32: log what kills the press-now ghost on every no-save
        bool captureKiller = false;  // v2.30: record what kills the ghost in the press-now run
        bool baseTouchedDown = false;  // v2.63: the press-now ghost was back on the ground at some point
        bool killerSeen = false;
        int killerTick = -1;
        int killerId = 0, killerType = -1;
        bool killerNoTouch = false, killerDisabled = false;
        CCRect killerRect, ghostDeathRect;
        bool robustSaves = true;     // v2.28: a save must also survive with you slightly off
        float ghostShiftX = 0.f;     // v2.28: horizontal offset applied to the ghost's start
        int lateCrash = 90;          // v2.27: cube / ball crashes after this many ticks aren't this click's (0 = off)
        bool predictMovers = false;  // v2.27: move objects along their current motion during predictions
        bool predictTriggers = true; // v2.81: objects that a move trigger ahead will start moving
        int triggersAhead = 0;       // v2.81: move triggers the last prediction looked at (explain log)
        bool shipBuffer = false;     // v2.25: buffer ship inputs too
        int shipSpamGap = 6;         // v2.44: ship feathering is normal control, only taps closer than this are spam
        int shipMaxDelay = 2;        // v2.25: ship's own max delay (240/s ticks)
        // v2.88 per-mode frames (menu's Custom tab): each game mode's own max delay, in 240/s
        // ticks, used instead of the stage's when on. Order: MODE_NAMES.
        bool perMode = false;
        std::array<int, 8> modeTicks { 40, 4, 40, 40, 20, 40, 40, 40 };
        // v3.0 Modes page: bit (mode * 2 + mini) set = that mode isn't buffered at all (raw clicks)
        uint32_t modeOff = 0;
        bool perfLog = false;        // v2.21: log time / runs / ghost ticks per click
        int simRuns = 0;             // v2.21: predictions run for the current click
        long simTicks = 0;           // v2.21: ghost ticks stepped for the current click
        bool cbfSync = true;         // v2.18: line the ghost up with Click Between Frames' mid-tick clicks
        float firstStep = 1.f;       // v2.18: fraction of a tick the ghost's first step lasts
        bool inRealUpdate = false;   // v2.18: inside P1's physics step (a CBF click lands here)
        float tickStartX = 0.f;      // v2.18: P1's x at the start of the current tick
        float tickDx = 0.f;          // v2.18: how far P1 moved in the last full tick
        bool dashAutoRelease = false;  // v2.16: let go of a dash for you when it would crash
        bool autoDash = false;       // v2.16: the current auto-release ended a dash
        std::unordered_set<GameObject*> orbsUsed;  // v2.16: orbs the ghost actually used this run
        bool moreTime = true;        // v2.43: at spots you keep dying, delay to give you more time
        bool ballSmart = true;       // v2.45: ball landing rules + spam guard
        bool swingSmart = true;      // v2.51: swing "could you have flipped back" scoring + spam guard
        bool train = false;          // v2.51: training mode: predict, never delay, show how early you were
        bool perLevel = true;        // v2.51: stage + switches remembered per level
        bool heatmap = true;         // v2.51: death marks on the progress bar
        bool saveDing = true;        // v2.56: ding when a click is saved
        CCLabelBMFont* hintLabel = nullptr;  // v2.51: training hint ("2t early")
        CCDrawNode* heatNode = nullptr;      // v2.51: death marks on the progress bar
        bool ufoSmart = true;        // v2.40: UFO landing + "could you have flapped again" scoring
        bool spiderSmart = true;     // v2.15: spider-aware landing + "could you have flipped back" scoring
        int retapAt = -1;            // v2.15: ticks after the input at which the ghost taps again (-1 = never)
        bool robotSmart = true;      // v2.12: robot presses count as safe if some hold length survives
        bool robotAutoRelease = false;  // v2.12: robot auto-release (practice mode only)
        bool autoNormalRuns = false; // v2.14: let auto-release work outside practice too
        CCLabelBMFont* autoTag = nullptr;  // v2.14: on-screen "AUTO-RELEASE ON" tag
        CCLabelBMFont* watermark = nullptr;  // v2.46: always-on "Click Before Frames" label while the buffer is on
        bool usedThisAttempt = false;      // v2.46: the buffer was on at some point of this attempt
        bool learn = true;           // v2.13: per-level memory learned from your runs
        bool fullState = true;       // v2.11.1: copy the full v2.8 player state into the ghost
        int orbWindow = 16;          // v2.6: how far ahead (ticks) an orb may be for that
        int simTick = 0;             // v2.6: tick of the current run
        int firstOrbTick = -1;       // v2.6: first tick the current run touched an orb
        int lastOrbTick = -1;        // v2.78: last tick the current run was inside an orb
        bool orbAfterFirst = false;  // v2.78: orb alignment: once your click used an orb, click the later ones too
        bool laterOrbTouched = false;  // v2.78: the run reached another orb after the one your click used
        int orbUsedTick = -1;        // v2.6: first tick the current run actually used an orb
        bool driftCheck = false;     // v2.4: log how far the real player strays from the prediction
        std::vector<CCPoint>* recordPath = nullptr;  // v2.4: ghost1 positions of the current run
        bool showPath = false;       // v2.11: draw the predicted path (F10)
        CCDrawNode* pathNode = nullptr;  // v2.11: owned by the level's object layer
        // v3.0 test 26: spots where autoplay really died although the ghost thought it was safe
        // (something the ghost can't see). Its ghost counts touching them as a death.
        std::vector<CCRect> autoDangers;
        // v3.0 test 27, the route: a prediction can start from a saved hidden-player state further
        // along (simFrom, simOffset ticks from now) instead of from you, and save states on the way
        PlayerObject* simFrom = nullptr;
        bool simFromHeld = false;
        int simOffset = 0;
        int cpEvery = 0;
        void (*cpSink)(int, PlayerObject*, bool) = nullptr;
        std::vector<CCPoint> const* crossPos = nullptr;  // the route's positions (for triggers it already passed)
        int crossBase = 0;                               // crossPos[k] is after tick crossBase + k (from now)
        // v3.0 test 27: GD only gives a player the objects near the camera; a hidden player past the
        // screen gets the rest from this index of the level (built when autoplay starts)
        bool offscreenFill = false;
        std::unordered_map<int, std::vector<GameObject*>> objIndex;
        std::unordered_set<GameObject*> seenThisCheck;
        bool inCheck = false;
        // v3.0 test 45: wave / ship routes stay in a band around where the stretch starts (leaving it
        // counts as a death in the route's tests), so the route flies steadily instead of to the
        // top and bottom of every corridor
        // v3.0 test 50: the floor / ceiling a mode portal puts around you (ship, UFO, wave, swing,
        // ball, spider), for the route's ghost: two invisible solid slabs it collides with
        bool gBounds = false;
        float gBoundLo = 0.f, gBoundHi = 0.f;
        bool simFromBounds = false;
        float simFromLo = 0.f, simFromHi = 0.f;
        GameObject* boundSlabs[2] = { nullptr, nullptr };
        int portalLogs = 0;
        bool yBand = false;
        float yBandCenter = 0.f;
        float yBandHalf = 0.f;
        bool postCollisionSeen = false;  // test 33: GD called postCollision inside this collision check
        long long offscreenAdded = 0;    // objects the index added (shown at a death)
        long long offscreenFallback = 0; // ...of those, added after the check (postCollision never came)
        int pathHoldUntil = 0;       // v2.65: live path waits while a click's path is on screen

        std::unordered_set<GameObject*> simUsed;
        std::vector<ObjSnapshot> snapshots;
    } buf;

    std::string stageText();
    void forgetMoveTriggers();
