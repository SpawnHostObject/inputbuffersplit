//@ ui/buffer_menu/buffer_menu.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
class BufferMenu : public geode::Popup {
    struct Option {
        char const* key;    // mod setting, or nullptr for the runtime path view
        char const* label;
        char const* sub;
    };
    static constexpr Option PREDICTION[] = {
        { "robust-saves", "Robust saves", "Only hold a click when it surely helps" },
        { "orb-align", "Orb alignment", "Lines orb clicks up with the orb" },
        { "predict-moving-objects", "Moving objects", "Predicts moving blocks and spikes" },
        { "predict-move-triggers", "Move triggers", "Follows triggers that move the level" },
        { "predict-through-portals", "Through portals", "Predictions go through mode portals" },
        { "plan-next-click", "Two clicks ahead", "Plans for your next click too" },
        { "no-hitch-searches", "No-hitch searches", "Spreads heavy searches over frames" },
        { "level-memory", "Level memory", "Learns the spots you die at" },
        { "more-time-death-spots", "More time at deaths", "Holds a bit longer where you die" },
    };
    static constexpr Option HELPERS[] = {
        { "low-latency", "Low latency", "At most 1 frame of delay" },
        { "cbf-sync", "CBF sync", "Lines up with Click Between Frames" },
        { "ship-buffer", "Ship buffering", "Buffers ship clicks too" },
        { "robot-smart", "Smart robot", "Robot hold length" },
        { "spider-smart", "Smart spider", "Spider teleports" },
        { "ufo-smart", "Smart UFO", "UFO jumps" },
        { "ball-smart", "Smart ball", "Ball flips" },
        { "swing-smart", "Smart swing", "Swing flips" },
    };
    static constexpr Option STAGE_OPTIONS[] = {
        { "orb-timing", "Orb Timing", "Presses on the safe ticks of an orb" },
        { "explain-inputs", "Log: More Detail", "Every click's decision in the Log" },
        { "wave-auto-release", "Wave Auto-Release", "Practice: lets go before a crash" },
    };
    static constexpr Option ASSISTS[] = {
        { nullptr, "Show Path", "Your path and the predicted one" },
        { "training-mode", "Training Hints", "Tells you when you were early" },
        { "death-heatmap", "Death Heatmap", "Your deaths on the progress bar" },
        { "save-ding", "Ding On Save", "A sound when a click is saved" },
        { "performance-mode", "Performance Mode", "Lighter searches for weak phones" },
    };
    static constexpr char const* TABS[] = { "Log", "Stage", "Modes", "Tools", "Account" };  // test 91: Account
    static constexpr int TAB_ACCOUNT = 4;
    static constexpr int TAB_TUNING = 5;
    static constexpr int TAB_SETTINGS = 6;
    // stage buttons: Off, then the stages from small to big, then Custom (per-mode frames)
    static constexpr int STAGE_BTN[] = { 7, 0, 1, 2, 3, 4, 5 };
    static constexpr char const* STAGE_LABEL[] = { "Off", "0.5f", "1f", "2f", "4f", "6f", "8f" };

    struct Theme {
        ccColor3B main, soft, accent, bg;
        char const* button;
    };

public:
    static constexpr ccColor3B ACCENTS[] = { { 166, 107, 255 }, { 90, 150, 255 }, { 50, 200, 225 }, { 80, 205, 120 },
        { 255, 160, 60 }, { 255, 95, 140 } };
    static constexpr ccColor3B BACKS[] = { { 18, 12, 34 }, { 10, 16, 38 }, { 8, 26, 30 }, { 10, 28, 14 }, { 34, 20, 10 },
        { 24, 24, 28 }, { 30, 14, 38 } };

    static ccColor3B hueColor(int hue) {
        float h = static_cast<float>(((hue % 360) + 360) % 360) / 60.f;
        float x = 1.f - std::abs(std::fmod(h, 2.f) - 1.f);
        float r = 0, g = 0, b = 0;
        switch (static_cast<int>(h)) {
            case 0: r = 1; g = x; break;
            case 1: r = x; g = 1; break;
            case 2: g = 1; b = x; break;
            case 3: g = x; b = 1; break;
            case 4: r = x; b = 1; break;
            default: r = 1; b = x; break;
        }
        auto c = [](float v) { return static_cast<GLubyte>(70 + v * 185); };
        return { c(r), c(g), c(b) };
    }

    static ccColor3B accentColor() {
        if (!Mod::get()->hasSavedValue("ui-accent")) {
            return { 255, 46, 59 };  // v3.2: the website's red
        }
        int i = Mod::get()->getSavedValue<int>("ui-accent", 0);
        if (i >= 0 && i < static_cast<int>(std::size(ACCENTS))) {
            return ACCENTS[i];
        }
        return hueColor(Mod::get()->getSavedValue<int>("ui-accent-hue", 262));
    }

    // a background colour made bright enough to see as a dot
    static ccColor3B bright(ccColor3B c) {
        auto k = [](GLubyte v) { return static_cast<GLubyte>(std::min(255, v * 3 + 20)); };
        return { k(c.r), k(c.g), k(c.b) };
    }

    static ccColor3B mix(ccColor3B a, ccColor3B b, float t) {
        auto m = [t](GLubyte x, GLubyte y) { return static_cast<GLubyte>(x + (y - x) * t); };
        return { m(a.r, b.r), m(a.g, b.g), m(a.b, b.b) };
    }

    // the accent colour picked in the menu (the old "Menu theme" setting is no longer used)
    static Theme currentTheme() {
        auto a = accentColor();
        return { a, mix(a, { 255, 255, 255 }, 0.6f), mix(a, { 255, 255, 255 }, 0.4f), mix(a, { 255, 255, 255 }, 0.5f), "GJ_button_02.png" };
    }

private:
    static constexpr float W = 460.f;
    static constexpr float H = 286.f;
    static constexpr float SIDE = 120.f;
    static constexpr float HEAD = 32.f;
    static constexpr float FOOT = 22.f;

    static inline BufferMenu* s_open = nullptr;
    static inline int s_tab = 1;
    static inline int s_back = 1;
    static inline bool s_moved = false;
    static inline CCPoint s_pos {};

    Theme m_t = currentTheme();
    bool m_light = Mod::get()->getSavedValue<bool>("ui-light", false);
    int m_bg = Mod::get()->getSavedValue<int>("ui-bg", 0);
    ccColor3B m_text { 232, 234, 255 };
    ccColor3B m_muted { 142, 148, 190 };
    ccColor3B m_cyan { 110, 230, 255 };

    CCMenu* m_frameMenu = nullptr;
    CCNode* m_frame = nullptr;
    CCNode* m_tabsNode = nullptr;
    CCNode* m_page = nullptr;
    CCMenu* m_pageMenu = nullptr;
    CCLabelBMFont* m_chip = nullptr;
    CCLabelBMFont* m_tpsLabel = nullptr;
    CCLabelBMFont* m_scaleLabel = nullptr;
    CCLabelBMFont* m_opacityLabel = nullptr;
    std::vector<Option const*> m_options;
    std::vector<CCDrawNode*> m_switches;
    std::vector<CCLabelBMFont*> m_switchLabels;
    std::array<CCLabelBMFont*, 8> m_modeValues {};
    bool m_dragging = false;
    float m_shootTimer = 2.f;
    CCNode* m_sky = nullptr;
    // v3.2: the site's look: TV static, an eye in the logo that follows the cursor, eyes in the snow
    CCSprite* m_static = nullptr;
    float m_staticT = 0.f;
    CCNode* m_eye = nullptr;
    CCNode* m_eyeIris = nullptr;
    float m_eyeW = 0.f, m_eyeX = 0.f, m_eyeY = 0.f, m_eyeTX = 0.f, m_eyeTY = 0.f, m_eyeWander = 0.f;
    float m_blinkIn = 2.5f, m_blink = -1.f;
    float m_ghostIn = 3.f;

#include "ui/buffer_menu/drawing.inl"

#include "ui/buffer_menu/frame.inl"

#include "ui/buffer_menu/pages.inl"

#include "ui/buffer_menu/motion.inl"

#include "ui/buffer_menu/refresh.inl"

#include "ui/buffer_menu/update.inl"

#include "ui/buffer_menu/touch.inl"

#include "ui/buffer_menu/actions.inl"

#include "ui/buffer_menu/lifecycle.inl"
};
