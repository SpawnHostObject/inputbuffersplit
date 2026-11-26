//@ input/queue.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    void queueInput(int ticks, bool down) {
        buf.queue.push_back({ ticks, down });
    }
