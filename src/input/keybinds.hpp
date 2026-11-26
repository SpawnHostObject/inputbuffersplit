//@ input/keybinds.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    void keyAction(int action) {
        auto* layer = GameManager::get()->m_playLayer;
        if (!layer) {
            return;
        }
        switch (action) {
            case 0:
                cycleStage();
                break;
            case 1:
                if (!layer->m_isPaused) {
                    layer->pauseGame(true);
                }
                BufferMenu::open();
                break;
            case 2:
                exportConsole();
                break;
            case 3:
                buf.panelOpen = !buf.panelOpen;
                refreshPanel();
                break;
            case 4:
                buf.showPath = !buf.showPath;
                if (buf.pathNode) {
                    buf.pathNode->clear();
                }
                say("path view {}", buf.showPath ? "on" : "off");
                break;
            case 5:
                memory.clear();
                pending.active = false;
                memoryDirty = true;
                saveMemory();
                say("memory for this level cleared");
                break;
        }
    }

    void setupKeybinds() {
        char const* binds[] = { "bind-stage", "bind-menu", "bind-export", "bind-console", "bind-path", "bind-forget" };
        for (int i = 0; i < 6; ++i) {
            listenForKeybindSettingPresses(binds[i], [i](Keybind const&, bool down, bool repeat, double) {
                if (down && !repeat) {
                    keyAction(i);
                }
                return false;
            });
        }
    }
