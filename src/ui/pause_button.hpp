//@ ui/pause_button.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
// v2.94: the pause menu has a button for the menu again (v2.92 had dropped it, so on phones,
// with no menu key, there was no way to open it inside a level)
// v2.99.4: the pause menu button is added by one function, from two places. On iOS the game
// builds the pause menu with its setup inlined, so the customSetup hook alone never ran there;
// PlayLayer::pauseGame (below) adds it after the pause menu is shown. Never added twice.
static void addPauseButton(CCNode* pause) {
    if (!pause || pause->getChildByID("click-before-frames-menu"_spr)) {
        return;
    }
    // v3.0: can be switched off in the menu's Settings (the menu key still opens it)
    if (!Mod::get()->getSavedValue<bool>("pause-button", true)) {
        return;
    }
    // test 91: the CBF icon (was a heart on a purple circle)
    auto* sprite = cbfIcon(42.f);
    if (!sprite) {
        return;
    }
    auto* button = CCMenuItemExt::createSpriteExtra(sprite, [](CCMenuItemSpriteExtra*) {
        BufferMenu::open();
    });
    button->setID("click-before-frames-button"_spr);
    auto* menu = CCMenu::create();
    menu->addChild(button);
    auto win = CCDirector::get()->getWinSize();
    menu->setPosition({ win.width - 40.f, 40.f });
    menu->setID("click-before-frames-menu"_spr);
    pause->addChild(menu, 10);
}

class $modify(BufPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        addPauseButton(this);
    }
};
