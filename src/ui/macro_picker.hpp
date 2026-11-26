//@ ui/macro_picker.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
// v2.88 menu: a draggable window in a space theme (night-sky gradient, nebula, twinkling
// stars, the odd shooting star) with a tab for each category: Buffer, Prediction, Modes,
// Custom (each game mode's own frames) and Tools. Opens with the menu key in a level or the
// heart button on the main menu.
// v3.0 menu: a sidebar (CBF logo, Log / Stage / Modes / Tools, theme picker, platform), a header
// with the stage chip, a footer with the settings gear. Settings and Tuning are pages of their own
// with a Done button. Everything is drawn in code (CCDrawNode), so it looks the same everywhere.
// alpha.31: Choose Macro: the macros of this level on every site, the player picks one
class MacroPicker : public geode::Popup {
    int m_level = 0;
    std::vector<MacroChoice> m_list;
    CCLabelBMFont* m_status = nullptr;
    geode::ScrollLayer* m_scroll = nullptr;
    std::shared_ptr<bool> m_alive = std::make_shared<bool>(true);

    static constexpr float PW = 400.f, PH = 260.f;

public:
    ~MacroPicker() override { *m_alive = false; }

    static MacroPicker* create() {
        auto* ret = new MacroPicker();
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    bool init() {
        if (!Popup::init(PW, PH, "GJ_square01.png")) {
            return false;
        }
        this->setTitle("Choose Macro");
        auto* pl = GameManager::get()->m_playLayer;
        if (!pl || !pl->m_level) {
            return false;
        }
        m_level = pl->m_level->m_levelID.value();
        std::string name = pl->m_level->m_levelName;
        std::string print = levelFingerprint(pl->m_level);
        int tps = static_cast<int>(settingTps());
        m_status = CCLabelBMFont::create("Looking on the global brain, Hyperbolus, MacroDash and Astral...", "chatFont.fnt");
        m_status->setScale(0.7f);
        m_status->setPosition({ PW / 2.f, PH / 2.f });
        m_mainLayer->addChild(m_status);
        auto alive = m_alive;
        int level = m_level;
        std::thread([alive, level, name, print, tps, this] {
            auto list = listMacroChoices(level, name, print, tps);
            queueInMainThread([alive, list, this] {
                if (*alive) {
                    showList(list);
                }
            });
        }).detach();
        return true;
    }

    void showList(std::vector<MacroChoice> const& list) {
        m_list = list;
        if (m_list.empty()) {
            m_status->setString("No site has a macro of this level.");
            return;
        }
        m_status->setVisible(false);
        float vw = PW - 30.f, vh = PH - 60.f, rowH = 34.f;
        m_scroll = geode::ScrollLayer::create({ vw, vh });
        m_scroll->setPosition({ 15.f, 15.f });
        m_mainLayer->addChild(m_scroll);
        float total = std::max(vh, rowH * m_list.size());
        m_scroll->m_contentLayer->setContentSize({ vw, total });
        auto* menu = CCMenu::create();
        menu->setPosition({ 0.f, 0.f });
        menu->setContentSize({ vw, total });
        m_scroll->m_contentLayer->addChild(menu);
        for (size_t i = 0; i < m_list.size(); ++i) {
            auto const& c = m_list[i];
            float y = total - rowH * (i + 0.5f);
            std::string top = fmt::format("[{}] {}", c.o.source, c.o.file);
            std::string sub;
            if (c.brain) {
                sub = fmt::format("{} inputs, replayed to the end {} time{}, failed {}", c.inputs, c.replays, c.replays == 1 ? "" : "s", c.fails);
            }
            else {
                if (!c.o.author.empty()) sub += "by " + c.o.author;
                if (c.tps > 0) sub += fmt::format("{}{} TPS", sub.empty() ? "" : "  ", c.tps);
                if (c.o.gameVersion > 0.0) sub += fmt::format("{}GD {:.4g}", sub.empty() ? "" : "  ", c.o.gameVersion);
                if (c.o.downloads > 0) sub += fmt::format("{}{} downloads", sub.empty() ? "" : "  ", c.o.downloads);
                if (c.o.approved) sub += "  approved";
                if (!c.o.notes.empty()) sub += "  \"" + c.o.notes.substr(0, 40) + "\"";
            }
            auto* t = CCLabelBMFont::create(top.c_str(), "bigFont.fnt");
            t->setAnchorPoint({ 0.f, 0.5f });
            t->setPosition({ 4.f, y + 6.f });
            t->limitLabelWidth(vw - 80.f, 0.4f, 0.1f);
            menu->addChild(t);
            auto* s = CCLabelBMFont::create(sub.c_str(), "chatFont.fnt");
            s->setAnchorPoint({ 0.f, 0.5f });
            s->setPosition({ 4.f, y - 8.f });
            s->limitLabelWidth(vw - 80.f, 0.55f, 0.1f);
            s->setColor({ 200, 200, 220 });
            menu->addChild(s);
            auto* spr = ButtonSprite::create("Use", "goldFont.fnt", "GJ_button_01.png", 0.6f);
            auto* b = CCMenuItemSpriteExtra::create(spr, this, menu_selector(MacroPicker::onUse));
            b->setTag(static_cast<int>(i));
            b->setPosition({ vw - 30.f, y });
            menu->addChild(b);
        }
        m_scroll->scrollToTop();
    }

    void onUse(CCObject* sender) {
        int i = static_cast<CCNode*>(sender)->getTag();
        if (i < 0 || i >= static_cast<int>(m_list.size())) {
            return;
        }
        useMacroChoice(m_level, m_list[i]);
        this->onClose(nullptr);
    }
};
