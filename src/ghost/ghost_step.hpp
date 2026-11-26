//@ ghost/ghost_step.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    void advanceGhost(PlayerObject* ghost, float dt) {
        ghost->update(dt);
        ghost->updateRotation(dt);
    }

    // v3.0 test 27: the level index's objects near a hidden player that GD didn't give it this step
    void injectOffscreen(PlayLayer* layer, PlayerObject* ghost, float dt) {
        static gd::vector<GameObject*> extra;
        extra.clear();
        auto pos = ghost->getPosition();
        int b = static_cast<int>(std::floor(pos.x / 64.f));
        for (int k = b - 3; k <= b + 3; ++k) {
            auto it = buf.objIndex.find(k);
            if (it == buf.objIndex.end()) {
                continue;
            }
            for (auto* o : it->second) {
                if (std::abs(o->m_positionX - pos.x) < 200.f && std::abs(o->m_positionY - pos.y) < 200.f && !buf.seenThisCheck.contains(o)) {
                    extra.push_back(o);
                }
            }
        }
        if (buf.gBounds) {
            // test 50: the corridor's floor and ceiling, under / over the ghost
            for (int i = 0; i < 2; ++i) {
                auto*& slab = buf.boundSlabs[i];
                if (!slab) {
                    slab = GameObject::createWithKey(1);
                    if (!slab) {
                        continue;
                    }
                    slab->retain();
                    slab->setScaleX(40.f);
                }
                float y = i == 0 ? buf.gBoundLo - 15.f : buf.gBoundHi + 15.f;
                slab->setPosition({ pos.x, y });
                slab->m_positionX = pos.x;
                slab->m_positionY = y;
                slab->setObjectRectDirty(true);
                slab->m_isOrientedBoxDirty = true;
                extra.push_back(slab);
            }
        }
        if (!extra.empty()) {
            buf.offscreenAdded += static_cast<long long>(extra.size());
            layer->collisionCheckObjects(ghost, &extra, static_cast<int>(extra.size()), dt);
        }
    }

    void checkGhost(PlayLayer* layer, PlayerObject* ghost, float dt) {
        ghost->m_collisionLogTop->removeAllObjects();
        ghost->m_collisionLogBottom->removeAllObjects();
        ghost->m_collisionLogLeft->removeAllObjects();
        ghost->m_collisionLogRight->removeAllObjects();
        buf.seenThisCheck.clear();
        buf.inCheck = true;
        buf.postCollisionSeen = false;
        layer->checkCollisions(ghost, dt, false);
        buf.inCheck = false;
        if (buf.simulating && buf.gBounds && !buf.ghostDied && ghost->m_isDart) {
            // test 53: the wave dies on the corridor's floor / ceiling (your "object 0" deaths)
            auto r = ghost->getObjectRect();
            if (r.getMaxY() >= buf.gBoundHi || r.getMinY() <= buf.gBoundLo) {
                buf.ghostDied = true;
            }
        }
        if (buf.offscreenFill && !buf.postCollisionSeen && !buf.objIndex.empty() && !buf.ghostDied) {
            // test 33: GD didn't call postCollision in this check: the far objects go in after it
            auto before = buf.offscreenAdded;
            injectOffscreen(layer, ghost, dt);
            buf.offscreenFallback += buf.offscreenAdded - before;
        }
        if (buf.simulating && !buf.ghostDied && !buf.autoDangers.empty()) {
            auto r = ghost->getObjectRect();
            for (auto const& d : buf.autoDangers) {
                if (r.intersectsRect(d)) {
                    buf.ghostDied = true;
                    break;
                }
            }
        }
    }

    bool isFlying(PlayerObject* player) {
        return player->m_isDart || player->m_isBird || player->m_isSwing || player->m_isShip;
    }

    // Orb auto-clicks last one ghost update, then let go (unless the test itself is holding).
    void releaseAutoClicks() {
        if (buf.auto1) {
            if (!buf.held1 && buf.ghost1) {
                buf.ghost1->releaseButton(PlayerButton::Jump);
            }
            buf.auto1 = false;
        }
        if (buf.auto2) {
            if (!buf.held2 && buf.ghost2) {
                buf.ghost2->releaseButton(PlayerButton::Jump);
            }
            buf.auto2 = false;
        }
    }
