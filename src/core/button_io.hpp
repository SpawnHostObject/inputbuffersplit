//@ core/button_io.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // v2.85: what the game itself thinks: is the jump button down for this player?
    bool gameHolds(PlayerObject* player) {
        if (!player) {
            return false;
        }
        auto it = player->m_holdingButtons.find(static_cast<int>(PlayerButton::Jump));
        return it != player->m_holdingButtons.end() && it->second;
    }

    void syncHeld(PlayLayer* layer) {
        resetPending();
        buf.realInput = buf.appliedInput = gameHolds(layer->m_player1);
    }

    void sendButton(PlayLayer* layer, bool down) {
        buf.injecting = true;
        buf.appliedInput = down;
        layer->handleButton(down, 1, buf.isPlayer1);
        buf.injecting = false;
    }

    void pressGhost(PlayerObject* ghost, bool down) {
        (ghost == buf.ghost1 ? buf.held1 : buf.held2) = down;
        if (down) {
            ghost->pushButton(PlayerButton::Jump);
        }
        else {
            ghost->releaseButton(PlayerButton::Jump);
        }
    }
