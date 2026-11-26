//@ autoplay/macro_fixes.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // test 89 (as xdBot plays its macros): the bot's position record nearest where you are (by x, the
    // part that doesn't drift) puts you back on the macro's height and angle when you're slightly off,
    // so small physics differences don't add up into a death
    // alpha.10: the bot's height and angle at exactly this x, between its two records around it
    // (the nearest record alone was up to half a record off; with a 360 TPS macro at 240 that's enough
    // for Grief's mini wave to graze a spike)
    bool macroRecordAt(float x, float& y, float& r, bool second) {
        auto& fx = autoplay.macroFixes;
        if (fx.size() < 2) {
            return false;
        }
        if (autoplay.fixPtr >= fx.size() || fx[autoplay.fixPtr].x > x + 50.f) {
            autoplay.fixPtr = 0;
        }
        while (autoplay.fixPtr + 1 < fx.size() && fx[autoplay.fixPtr + 1].x <= x) {
            ++autoplay.fixPtr;
        }
        auto const& a = fx[autoplay.fixPtr];
        // alpha.14: player 2's record too (dual parts)
        auto ya = [&](MacroFix const& f) { return second ? f.y2 : f.y; };
        auto ra = [&](MacroFix const& f) { return second ? f.r2 : f.r; };
        if (second && !a.p2) {
            return false;
        }
        if (autoplay.fixPtr + 1 >= fx.size()) {
            y = ya(a);
            r = ra(a);
            return std::abs(a.x - x) <= 2.5f;
        }
        auto const& b = fx[autoplay.fixPtr + 1];
        float span = b.x - a.x;
        if (x < a.x - 2.5f || span <= 0.f || span > 5.f || (second && !b.p2)) {
            if (std::abs(a.x - x) > 2.5f) {
                return false;
            }
            y = ya(a);
            r = ra(a);
            return true;
        }
        float t = std::clamp((x - a.x) / span, 0.f, 1.f);
        y = ya(a) + (ya(b) - ya(a)) * t;
        float dr = std::remainder(ra(b) - ra(a), 360.f);
        r = ra(a) + dr * t;
        return true;
    }

    // alpha.14: you (and player 2 in a dual part) put on the bot's spot at this x. Alpha.13 left dual
    // parts out, and Grief's 36.1% is one: the nudges never happened
    bool macroPlaceAt(PlayLayer* layer, PlayerObject* p1, float x) {
        float y, r;
        if (!macroRecordAt(x, y, r) || std::abs(y - p1->getPositionY()) > 20.f) {
            return false;
        }
        float shift = x - p1->getPositionX();
        p1->setPosition({ x, y });
        p1->setRotation(r);
        auto* p2 = layer->m_player2;
        if (layer->m_gameState.m_isDualMode && p2 && !p2->m_isDead) {
            float y2, r2;
            if (macroRecordAt(x, y2, r2, true) && std::abs(y2 - p2->getPositionY()) <= 20.f) {
                p2->setPosition({ p2->getPositionX() + shift, y2 });
                p2->setRotation(r2);
            }
            else {
                p2->setPositionX(p2->getPositionX() + shift);
            }
        }
        return true;
    }

    void macroApplyFix(PlayLayer* layer, PlayerObject* p1, int now) {
        if (autoplay.macroFixes.empty() || !macroFixesSetting) {
            return;
        }
        auto& fx = autoplay.macroFixes;
        float px = p1->getPositionX();
        MacroFix const* best = nullptr;
        bool dualNow = layer->m_gameState.m_isDualMode && layer->m_player2 && !layer->m_player2->m_isDead;
        // v3.1.1: a brain run's records (every 3 steps and at each click, same TPS): the bot's height
        // between the two records around your x, near this step
        if (autoplay.macroIsBrain) {
            auto it = std::lower_bound(fx.begin(), fx.end(), now - 8, [](MacroFix const& f, int v) { return f.frame < v; });
            for (; it != fx.end() && std::next(it) != fx.end() && it->frame <= now + 8; ++it) {
                auto const& a = *it;
                auto const& b = *std::next(it);
                if (a.x <= px && px <= b.x && b.x - a.x < 12.f && b.frame - a.frame <= 4) {
                    float t = b.x > a.x ? (px - a.x) / (b.x - a.x) : 0.f;
                    auto put = [&](PlayerObject* p, float ya, float yb, float ra, float rb) {
                        float y = ya + (yb - ya) * t;
                        float dy = y - p->getPositionY();
                        if (std::abs(dy) < 0.01f || std::abs(dy) > 20.f) {
                            return;
                        }
                        p->setPositionY(y);
                        p->setRotation(ra + std::remainder(rb - ra, 360.f) * t);
                        ++autoplay.macroFixCount;
                        autoplay.macroFixMax = std::max(autoplay.macroFixMax, std::abs(dy));
                    };
                    put(p1, a.y, b.y, a.r, b.r);
                    if (dualNow && a.p2 && b.p2) {
                        put(layer->m_player2, a.y2, b.y2, a.r2, b.r2);
                    }
                    break;
                }
            }
            return;
        }
        if (autoplay.macroForceX) {
            float y, r;
            if (macroRecordAt(px, y, r)) {
                float dy = y - p1->getPositionY();
                if (std::abs(dy) >= 0.01f && std::abs(dy) <= 60.f) {
                    p1->setPositionY(y);
                    p1->setRotation(r);
                    ++autoplay.macroFixCount;
                    autoplay.macroFixMax = std::max(autoplay.macroFixMax, std::abs(dy));
                }
            }
            // alpha.14: player 2 in a dual part too, at the same x
            if (dualNow && macroRecordAt(px, y, r, true)) {
                auto* p2 = layer->m_player2;
                float dy = y - p2->getPositionY();
                if (std::abs(dy) >= 0.01f && std::abs(dy) <= 60.f) {
                    p2->setPositionY(y);
                    p2->setRotation(r);
                }
            }
            return;
        }
        if (autoplay.macroForceX) {
            // v3.1.1: by x (its frames are another TPS's): a pointer that moves along with you
            if (autoplay.fixPtr >= fx.size() || fx[autoplay.fixPtr].x > px + 50.f) {
                autoplay.fixPtr = 0;
            }
            while (autoplay.fixPtr + 1 < fx.size() && fx[autoplay.fixPtr + 1].x <= px) {
                ++autoplay.fixPtr;
            }
            for (size_t i = autoplay.fixPtr; i < fx.size() && i < autoplay.fixPtr + 3; ++i) {
                if (!best || std::abs(fx[i].x - px) < std::abs(best->x - px)) {
                    best = &fx[i];
                }
            }
        }
        else {
            auto it = std::lower_bound(fx.begin(), fx.end(), now - 3, [](MacroFix const& f, int v) { return f.frame < v; });
            for (; it != fx.end() && it->frame <= now + 2; ++it) {
                if (!best || std::abs(it->x - px) < std::abs(best->x - px)) {
                    best = &*it;
                }
            }
        }
        if (!best || std::abs(best->x - px) > 2.5f) {
            return;  // no record here, or you're somewhere else (a teleport): leave it
        }
        // v3.1.1: never into a block or a spike (a 6-unit fix pushed Penumbral's wave into the ceiling:
        // a death with no object). The player's box at the new height is checked against what's there.
        // alpha.21: only a fix that puts you deep inside a block (4 units) or into a spike's hitbox is left
        // out. Alpha.4 counted touching too, so a cube on the ground or riding a block lost its fixes
        // (Heliopolis: 93 left out an attempt, dead at 7.1%; v3.1.0, with no check, finished it)
        auto blocked = [&](PlayerObject* p, float dy) {
            auto full = p->getObjectRect();
            full.origin.y += dy;
            auto r = full;
            r.origin.x += 4.f;
            r.origin.y += 4.f;
            r.size.width = std::max(1.f, r.size.width - 8.f);
            r.size.height = std::max(1.f, r.size.height - 8.f);
            int b = static_cast<int>(std::floor(p->getPositionX() / 64.f));
            for (int k = b - 1; k <= b + 1; ++k) {
                auto it = buf.objIndex.find(k);
                if (it == buf.objIndex.end()) {
                    continue;
                }
                for (auto* o : it->second) {
                    auto t = o->m_objectType;
                    if (o->m_isDisabled || o->m_isDisabled2) {
                        continue;
                    }
                    if (t == GameObjectType::Solid && o->getObjectRect().intersectsRect(r)) {
                        return true;
                    }
                    if ((t == GameObjectType::Hazard || t == GameObjectType::AnimatedHazard) && o->getObjectRect().intersectsRect(full)) {
                        return true;
                    }
                }
            }
            return false;
        };
        // alpha.25: as xdBot plays its macros: you're put on the record's spot, x and y (the x only when
        // it's close, so a teleport isn't undone), with no check against what's there. Alpha.4 left out
        // records that touched a spike's box in this game, and those were the ones near spikes: Society
        // (2, 14 left out) and Heliopolis (34) died there; v3.1.0, with no check, finished Heliopolis
        auto fix = [&](PlayerObject* p, float x, float y, float r) {
            float dy = y - p->getPositionY();
            float dx = x - p->getPositionX();
            if ((std::abs(dy) < 0.01f && std::abs(dx) < 0.01f) || std::abs(dy) > (autoplay.macroForceX ? 60.f : 20.f)) {
                return;
            }
            if (!autoplay.macroForceX && std::abs(dx) <= 2.5f) {
                p->setPosition({ x, y });
            }
            else {
                p->setPositionY(y);
            }
            p->setRotation(r);
            ++autoplay.macroFixCount;
            autoplay.macroFixMax = std::max(autoplay.macroFixMax, std::abs(dy));
        };
        (void)blocked;
        float shiftX = std::abs(best->x - p1->getPositionX()) <= 2.5f ? best->x - p1->getPositionX() : 0.f;
        fix(p1, best->x, best->y, best->r);
        if (best->p2 && layer->m_gameState.m_isDualMode && layer->m_player2 && !layer->m_player2->m_isDead) {
            auto* p2 = layer->m_player2;
            fix(p2, p2->getPositionX() + shiftX, best->y2, best->r2);
        }
    }
