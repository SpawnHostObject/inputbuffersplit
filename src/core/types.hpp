//@ core/types.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    struct QueuedInput {
        int ticksLeft;
        bool down;
        // v2.85 no-hitch: not an input yet, the search for it goes on when it comes due
        bool resume = false;
        int from = 0;      // first delay (from then) not tried yet
        int elapsed = 0;   // ticks since your click
        int resumes = 0;
    };

    struct ObjSnapshot {
        GameObject* object;
        bool isActivated;
        EffectGameObject* effect;
        bool activated;
        bool byP1;
        bool byP2;
        bool animated = false;  // v2.84.6: m_animationTriggered (a pad's animation played)
    };
