//@ decision/click_timer.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // v2.4: simulate() with the orb setting; if that run touched an orb and didn't make
    // it, also try the other choice (click it / skip it) and keep whichever lives longer,
    // like a player who only clicks the orb when they need it.
    // v2.57: hard time cap for one click's whole decision. The search loops have their own
    // budget, but orb clicks also ran unbudgeted work (orb alignment, the both-ways and late
    // orb clicks in every run, robust and more-time checks) and could take several frames.
    std::chrono::steady_clock::time_point clickStart;
    bool clickTimed = false;

    // v2.70: how long the current click has been processed so far (0 outside a click)
    double clickMs() {
        return clickTimed ? std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - clickStart).count() : 0.0;
    }

    // v2.78.4: 1.25x (was 2x): with the searches now sharing the click's time, a whole save
    // stays close to the budget
    bool clickOverTime() {
        return clickMs() > buf.budgetMs * 1.25;
    }

    // v2.78.4: what's left of the click's time for a search. Each search used to get the full
    // budget on its own, on top of the base run, memory try and orb checks before it, so a
    // wave save on GRIEF could take 2-3 frames at 240 fps.
    // v2.80.2: but always at least half the budget. Orb alignment (up to 1.75x since v2.79)
    // could use all of it, and the normal search then stopped after one delay ("prediction
    // budget hit at +1 ticks" on almost every click near an orb, Heliopolis).
    double searchBudget() {
        return clickTimed ? std::max(buf.budgetMs - clickMs(), buf.budgetMs * 0.5) : buf.budgetMs;
    }

    struct ClickTimer {
        ClickTimer() {
            clickStart = std::chrono::steady_clock::now();
            clickTimed = true;
        }
        ~ClickTimer() {
            clickTimed = false;
        }
    };
