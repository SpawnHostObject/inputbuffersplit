//@ core/on_load_services.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
//@ 
//@ 
//@ 
//@ 
//@ 
//@ 
//@ 
//@ 
$on_mod(Loaded) {
    CBF_STEP("loaded: keybinds");
    setupKeybinds();
    online::start();
    account::refresh();  // test 92: is this game linked (autoplay and updates need it)
    CBF_STEP("loaded: keybinds done");
    // v2.99.3: shows in the log whether the game reaches its first frame (iOS start crash hunt)
    queueInMainThread([] { CBF_STEP("first frame"); });
}
