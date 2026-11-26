//@ ghost/ghost_identity.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    bool isGhost(PlayerObject* p) {
        return p && (p == buf.ghost1 || p == buf.ghost2);
    }
