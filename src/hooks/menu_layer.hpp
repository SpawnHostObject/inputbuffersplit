//@ hooks/menu_layer.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
//@ 
//@ 
//@ 
//@ 
//@ 
//@ 
//@ 
// the main menu: a heart button that opens the menu (with the other buttons at the bottom),
// and the update check
class $modify(BufMenuLayer, MenuLayer) {
    bool init() {
        CBF_STEP("main menu: game's menu");
        if (!MenuLayer::init()) {
            return false;
        }
        CBF_STEP("main menu: heart button");
        auto* sprite = cbfIcon(46.f);  // test 91: the CBF icon (was a heart)
        if (!sprite) {
            geode::log::error("cbf-icon.png missing, no menu button");
            return true;
        }
        auto* item = CCMenuItemSpriteExtra::create(sprite, this, menu_selector(BufMenuLayer::onBufferMenu));
        item->setID("click-before-frames-button"_spr);
        if (auto* bottom = this->getChildByID("bottom-menu")) {
            bottom->addChild(item);
            bottom->updateLayout();
        }
        CBF_STEP("main menu: update check");
        checkForUpdate();
        CBF_STEP("main menu: done");
        if (!latestVersion.empty() && !updateShown) {
            queueInMainThread([] { showUpdatePopup(); });
        }
        return true;
    }

    void onBufferMenu(CCObject*) {
        BufferMenu::open();
    }
};
