//@ ui/buffer_menu/lifecycle.inl
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
protected:
    bool init() {
        if (!Popup::init(W, H, "GJ_square04.png")) {
            return false;
        }
        m_bgSprite->setVisible(false);
        if (m_closeBtn) {
            m_closeBtn->setVisible(false);
            m_closeBtn->setEnabled(false);
        }
        this->setOpacity(90);
        if (s_moved) {
            m_mainLayer->setPosition(s_pos);
        }
        s_open = this;
        s_sideOut = false;  // test 65: the autoplay panel slides out each time the window opens
        applyLook();
        buildFrame();
        rebuildPage();
        this->scheduleUpdate();
        return true;
    }

public:
    ~BufferMenu() override {
        if (s_open == this) {
            s_open = nullptr;
        }
    }

    static BufferMenu* create() {
        // v3.2: the menu starts at half size once (the UI Scale buttons still go from 0.4x to 1.3x)
        if (!Mod::get()->getSavedValue<bool>("ui-v32-scale", false)) {
            Mod::get()->setSavedValue("ui-scale", 0.5f);
            Mod::get()->setSavedValue("ui-v32-scale", true);
        }
        auto* ret = new BufferMenu();
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    // the menu key again closes it
    static void open() {
        if (s_open) {
            s_open->keyBackClicked();
            return;
        }
        if (auto* menu = create()) {
            menu->show();
        }
    }
