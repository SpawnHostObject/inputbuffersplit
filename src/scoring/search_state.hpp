//@ scoring/search_state.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // v2.1: best delayed run seen by the last search (shown in the "no save" log line)
    int dbgBestK = 0;
    int dbgBestRet = -1;
    // v2.85 no-hitch: the first delay a search tries (later than +1 when it goes on from a
    // previous tick), and where the last search ran out of time (0 = it didn't)
    int searchFrom = 1;
    int budgetCutK = 0;
    // set while a search is going on from a previous tick
    bool resumeNow = false;
    QueuedInput resumeEntry {};
    constexpr int MAX_RESUMES = 4;

    // v2.7 wave scoring, the wave version of the cube's "landed safely". A wave never
    // lands, so a run that keeps going one way eventually hits a wall even in a normal
    // corridor, where you'd simply turn. So: if the run crashes, try turning back a bit
    // before the crash; if that turn survives, you had time to react and the run counts
    // as safe. Only crashes you couldn't have turned away from count as deaths.
    // v2.10: three turn points are tried (4, 8 and 12 ticks before the crash) instead of
    // one, so "you could have turned away" is found whenever it's true.
    constexpr int WAVE_TURN_MARGINS[] = { 4, 8, 12 };
    constexpr int WAVE_MIN_TURN = 6;      // a turn sooner than this after the input is too fast to count
    constexpr int WAVE_ESCAPE = 16;       // the turn must then survive this many ticks
