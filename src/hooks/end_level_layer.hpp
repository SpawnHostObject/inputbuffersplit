//@ hooks/end_level_layer.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.

// v2.75.1: the level complete screen no longer shows "Click Before Frames was on" or the
// saves count in the top right corner; the attempt is still finished here.
class $modify(BufEndLevelLayer, EndLevelLayer) {
    void customSetup() {
        EndLevelLayer::customSetup();
        benchFinish();  // a completed run is the best recording there is
        benchFlush();
        endAttempt();
    }
};
