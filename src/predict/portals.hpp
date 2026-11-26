//@ predict/portals.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // v2.79 predict through portals: a mode portal the ghost reaches switches the ghost's own
    // mode (the player's toggles, no effects), and the prediction keeps going instead of
    // stopping there. Only the ghost changes; the level's floor / ceiling for the new mode
    // isn't set, so a ship or wave right after a portal is predicted without those bounds.
    // Dual portals still stop the prediction (v2.84: teleport and solo portals are taken, see
    // ghostTeleport). Returns false for those.
    bool boundedMode(GameObjectType t) {
        return t == GameObjectType::ShipPortal || t == GameObjectType::UfoPortal || t == GameObjectType::WavePortal ||
            t == GameObjectType::SwingPortal || t == GameObjectType::BallPortal || t == GameObjectType::SpiderPortal;
    }

    GameObjectType portalTypeOf(PlayerObject* p) {
        if (p->m_isShip) return GameObjectType::ShipPortal;
        if (p->m_isBird) return GameObjectType::UfoPortal;
        if (p->m_isDart) return GameObjectType::WavePortal;
        if (p->m_isSwing) return GameObjectType::SwingPortal;
        if (p->m_isBall) return GameObjectType::BallPortal;
        if (p->m_isSpider) return GameObjectType::SpiderPortal;
        if (p->m_isRobot) return GameObjectType::RobotPortal;
        return GameObjectType::CubePortal;
    }

    // the corridor's height: the game's own number when it gives a sensible one, else 10 blocks
    // (8 for ball and spider)
    float corridorHeight(GJBaseGameLayer* layer, GameObjectType t) {
        float h = layer ? layer->getGroundHeightForMode(static_cast<int>(t)) : 0.f;
        if (h > 120.f && h < 600.f) {
            return h;
        }
        return (t == GameObjectType::BallPortal || t == GameObjectType::SpiderPortal) ? 240.f : 300.f;
    }

    bool portalIsFree(GJBaseGameLayer* layer, GameObject* portal) {
        bool freeMode = layer && layer->m_gameState.m_isFreeMode;
        if (auto* e = typeinfo_cast<EffectGameObject*>(portal)) {
            freeMode = freeMode || e->m_cameraIsFreeMode;
        }
        return freeMode;
    }

    // v3.0 test 53: the game snaps the corridor to the block grid (a wave portal at y 268 made the
    // corridor 90-390: centre 240), and its floor is never below the ground (90). Test 50 centred it
    // on the portal itself, 28 units too high.
    void corridorFor(float portalY, float h, float& lo, float& hi) {
        float c = std::floor(portalY / 30.f) * 30.f;
        lo = std::max(90.f, c - h * 0.5f);
        hi = lo + h;
    }

    void ghostBoundsFromPortal(GameObject* portal, GameObjectType t) {
        auto* layer = GameManager::get()->m_playLayer;
        if (!boundedMode(t) || portalIsFree(layer, portal)) {
            buf.gBounds = false;
            return;
        }
        corridorFor(portal->getPositionY(), corridorHeight(layer, t), buf.gBoundLo, buf.gBoundHi);
        buf.gBounds = true;
    }

    // the corridor you're in for real right now
    void realBounds(PlayLayer* layer, PlayerObject* p, bool& has, float& lo, float& hi) {
        has = false;
        if (!layer || !p) {
            return;
        }
        auto t = portalTypeOf(p);
        auto* portal = layer->m_gameState.m_lastActivatedPortal1;
        if (!boundedMode(t) || !portal || portalIsFree(layer, portal)) {
            return;
        }
        // test 53: from the last portal you took (m_portalY read 0 in the game's own state)
        corridorFor(portal->getPositionY(), corridorHeight(layer, t), lo, hi);
        has = true;
    }

    bool ghostTakePortal(PlayerObject* g, GameObject* portal) {
        GameObjectType type = portal->m_objectType;
        switch (type) {
            case GameObjectType::CubePortal:
            case GameObjectType::ShipPortal:
            case GameObjectType::UfoPortal:
            case GameObjectType::BallPortal:
            case GameObjectType::WavePortal:
            case GameObjectType::RobotPortal:
            case GameObjectType::SpiderPortal:
            case GameObjectType::SwingPortal:
                break;
            default:
                return false;
        }
        bool already = (type == GameObjectType::ShipPortal && g->m_isShip) || (type == GameObjectType::UfoPortal && g->m_isBird) ||
            (type == GameObjectType::BallPortal && g->m_isBall) || (type == GameObjectType::WavePortal && g->m_isDart) ||
            (type == GameObjectType::RobotPortal && g->m_isRobot) || (type == GameObjectType::SpiderPortal && g->m_isSpider) ||
            (type == GameObjectType::SwingPortal && g->m_isSwing) ||
            (type == GameObjectType::CubePortal && !g->m_isShip && !g->m_isBird && !g->m_isBall && !g->m_isDart && !g->m_isRobot &&
                !g->m_isSpider && !g->m_isSwing);
        ghostBoundsFromPortal(portal, type);  // test 50 (a portal of the mode you're in sets it again)
        if (already) {
            return true;
        }
        if (g->m_isShip) g->toggleFlyMode(false, true);
        if (g->m_isBird) g->toggleBirdMode(false, true);
        if (g->m_isBall) g->toggleRollMode(false, true);
        if (g->m_isDart) g->toggleDartMode(false, true);
        if (g->m_isRobot) g->toggleRobotMode(false, true);
        if (g->m_isSpider) g->toggleSpiderMode(false, true);
        if (g->m_isSwing) g->toggleSwingMode(false, true);
        switch (type) {
            case GameObjectType::ShipPortal: g->toggleFlyMode(true, true); break;
            case GameObjectType::UfoPortal: g->toggleBirdMode(true, true); break;
            case GameObjectType::BallPortal: g->toggleRollMode(true, true); break;
            case GameObjectType::WavePortal: g->toggleDartMode(true, true); break;
            case GameObjectType::RobotPortal: g->toggleRobotMode(true, true); break;
            case GameObjectType::SpiderPortal: g->toggleSpiderMode(true, true); break;
            case GameObjectType::SwingPortal: g->toggleSwingMode(true, true); break;
            default: break;  // cube: every mode off
        }
        buf.ghostSwitched = true;
        return true;
    }

    // v2.84 teleport portals: the ghost jumps by the blue -> orange offset (how plain teleport
    // pairs move you) and keeps going instead of the prediction stopping there. Portals with
    // forces, redirects or gravity changes still stop it. Returns false for those.
    bool ghostTeleport(PlayerObject* g, GameObject* object) {
        if (object->m_objectType != GameObjectType::TeleportPortal) {
            return false;
        }
        if (object->m_objectID != 747) {
            return object->m_objectID == 749;  // touching the orange end does nothing
        }
        auto* t = static_cast<TeleportPortalObject*>(object);
        if (t->m_staticForceEnabled || t->m_redirectForceEnabled || t->m_redirectDash || t->m_gravityMode < 0 || t->m_gravityMode > 3) {
            return false;
        }
        // v3.0 test 46: a teleport that also sets gravity (1 normal, 2 flipped, 3 toggle); the route
        // stopped at those as "a portal it can't take" (Tidal Wave 14%)
        if (t->m_gravityMode != 0) {
            bool want = t->m_gravityMode == 2 ? true : t->m_gravityMode == 1 ? false : !g->m_isUpsideDown;
            if (want != g->m_isUpsideDown) {
                g->flipGravity(want, true);
            }
        }
        double dx = 0.0, dy = t->m_teleportYOffset;
        if (t->m_orangePortal && t->m_orangePortal != t) {
            dx = t->m_orangePortal->m_positionX - t->m_positionX;
            dy = t->m_orangePortal->m_positionY - t->m_positionY;
        }
        if (t->m_ignoreX) {
            dx = 0.0;
        }
        if (t->m_ignoreY) {
            dy = 0.0;
        }
        if (dx == 0.0 && dy == 0.0) {
            return true;
        }
        CCPoint shift { static_cast<float>(dx), static_cast<float>(dy) };
        CCPoint pos = g->getPosition() + shift;
        g->setPosition(pos);
        g->m_position = pos;
        g->m_lastPosition = g->m_lastPosition + shift;
        g->m_isOnGround = false;
        return true;
    }

    // v2.84 solo portals: with two players, only P1 goes on after it (the prediction used to
    // stop there); already solo, it changes nothing.
    bool ghostSoloPortal(GameObject* object) {
        if (object->m_objectType != GameObjectType::SoloPortal) {
            return false;
        }
        buf.ghostSolo = true;
        buf.simSecond = false;
        return true;
    }

    void copyState(PlayerObject* ghost, PlayerObject* real);
    void pressGhost(PlayerObject* ghost, bool down);

    // v2.85 dual portals: P2 appears where P1 is, same mode and size, upside down compared to
    // P1, holding what P1 holds, and both go on (the prediction used to stop there). Already
    // dual, it changes nothing.
    bool ghostDualPortal(PlayerObject* g, GameObject* object) {
        if (object->m_objectType != GameObjectType::DualPortal) {
            return false;
        }
        if (buf.simSecond) {
            return true;
        }
        if (g != buf.ghost1 || !buf.ghost2) {
            return false;
        }
        auto* g2 = buf.ghost2;
        float shift = buf.ghostShiftX;
        buf.ghostShiftX = 0.f;
        copyState(g2, g);
        buf.ghostShiftX = shift;
        g2->setPosition(g->getPosition());
        g2->m_position = g->m_position;
        g2->m_lastPosition = g->m_lastPosition;
        g2->flipGravity(!g->m_isUpsideDown, true);
        pressGhost(g2, buf.held1);
        buf.simSecond = true;
        buf.ghostSolo = false;
        return true;
    }
