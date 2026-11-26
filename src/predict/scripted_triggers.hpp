//@ predict/scripted_triggers.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // the triggers this prediction can reach, and the objects they move
    void collectScripted(GJBaseGameLayer* layer, int total) {
        scriptedMoves.clear();
        scriptedObjects.clear();
        buf.triggersAhead = 0;
        if (!buf.predictTriggers || triggersDistrusted || !buf.ghost1 || !layer->m_player1 || layer->m_player1->m_isGoingLeft) {
            return;
        }
        double px = buf.ghost1->getPositionX();
        double py = buf.ghost1->getPositionY();
        double pyLo = py, pyHi = py;
        if (buf.simFrom && layer->m_player1) {
            // v3.0 test 31: a route test from further along: the triggers from YOU on (test 27-30
            // only took them from the test's start, so ones between you and it never moved anything)
            double rx = layer->m_player1->getPositionX();
            double ry = layer->m_player1->getPositionY();
            px = std::min(px, rx);
            pyLo = std::min(py, ry);
            pyHi = std::max(py, ry);
        }
        double reach = (total + 2) * std::max(static_cast<double>(std::abs(buf.tickDx)), 2.0) + 30.0;
        auto& cache = scriptedCache;
        if (cache.layer == layer && cache.tick == tickCount && reach <= cache.reach && std::abs(px - cache.px) < 3.0) {
            scriptedMoves = cache.moves;
            scriptedObjects = cache.objects;
            buf.triggersAhead = static_cast<int>(scriptedMoves.size());
            return;
        }
        scanMoveTriggers(layer);
        float tickLength = buf.tickLength > 0.001f && buf.tickLength < 2.f ? buf.tickLength : TICK;
        std::unordered_map<GameObject*, size_t> index;
        // v2.84.4: only objects whose motion can come near the ghost's path (it was every object
        // within ~800 units either side, at any height)
        double pathMinX = px - 90.0, pathMaxX = px + reach + 90.0;
        double pathMinY = pyLo - 450.0, pathMaxY = pyHi + 450.0;
        auto nearPath = [&](GameObject* object, ScriptedMove const& m) {
            double x = object->m_positionX, y = object->m_positionY;
            double x0 = x, x1 = x, y0 = y, y1 = y;
            if (m.kind == TriggerKind::Move) {
                x0 = std::min(x, x + m.dx);
                x1 = std::max(x, x + m.dx);
                y0 = std::min(y, y + m.dy);
                y1 = std::max(y, y + m.dy);
            }
            else if (m.hasCenter) {
                double r = std::hypot(x - m.cx, y - m.cy);
                x0 = m.cx - r;
                x1 = m.cx + r;
                y0 = m.cy - r;
                y1 = m.cy + r;
            }
            constexpr double SIZE = 60.0;  // the object's own size
            return x1 + SIZE >= pathMinX && x0 - SIZE <= pathMaxX && y1 + SIZE >= pathMinY && y0 - SIZE <= pathMaxY;
        };
        // adds trigger t: armed = it fires at step `when` (negative: it fired that long ago),
        // else when the ghost gets to x, `when` steps later (a spawn delay)
        auto addMove = [&](EffectGameObject* t, double x, TriggerKind kind, bool armed, int when, bool touch = false,
                           CCRect touchRect = CCRect {}) {
            int elapsed = armed && when < 0 ? -when : -1;
            if (static_cast<int>(scriptedMoves.size()) >= scriptedTriggerLimit()) {
                return;
            }
            auto* group = layer->getGroup(t->m_targetGroupID);
            if (!group || group->count() == 0) {
                return;
            }
            ScriptedMove m;
            m.kind = kind;
            m.x = x;
            m.touch = touch && !armed;
            m.rect = touchRect;
            m.armed = armed;
            m.fireStep = armed ? when : -1;
            m.delay = armed ? 0 : when;
            if (kind == TriggerKind::Toggle) {
                m.group = group;
                m.on = t->m_activateGroup;
                scriptedMoves.push_back(m);
                return;
            }
            fillMove(layer, t, kind, tickLength, m);
            if (elapsed >= 0) {
                if (m.steps < 0.5 || elapsed >= m.steps) {
                    return;  // already finished
                }
                m.from = easeMove(m.easing, m.rate, elapsed / m.steps);
            }
            int move = static_cast<int>(scriptedMoves.size());
            bool any = false;
            for (auto* object : CCArrayExt<GameObject*>(group)) {
                if (!object || object == t || object->m_isDecoration || object->m_objectType == GameObjectType::Decoration ||
                    !nearPath(object, m)) {
                    continue;
                }
                auto [found, added] = index.try_emplace(object, scriptedObjects.size());
                if (added) {
                    if (scriptedObjects.size() >= scriptedObjectLimit()) {
                        index.erase(found);
                        continue;
                    }
                    bool mover = std::any_of(movers.begin(), movers.end(), [&](Mover const& mv) { return mv.object == object; });
                    scriptedObjects.push_back({ object, object->m_positionX, object->m_positionY, object->getPosition(),
                        object->getRotationX(), object->getRotationY(), mover });
                }
                scriptedObjects[found->second].moves.push_back(move);
                any = true;
            }
            if (any) {
                scriptedMoves.push_back(m);
            }
        };
        // moves / rotations already running (with "predict moving objects" on, those are
        // extrapolated instead)
        if (!buf.predictMovers) {
            for (auto fired = firedMoves.begin(); fired != firedMoves.end();) {
                auto* t = fired->first;
                int elapsed = tickCount - fired->second;
                if (elapsed < 0 || elapsed * tickLength > t->m_duration * 60.f + 1.f) {
                    fired = firedMoves.erase(fired);
                    continue;
                }
                bool rotate = t->m_objectID == 1346;
                if (plainTrigger(t, rotate)) {
                    addMove(t, 0.0, rotate ? TriggerKind::Rotate : TriggerKind::Move, true, -elapsed);
                }
                ++fired;
            }
        }
        // v2.85: what spawning a group sets off: its spawn-triggered move / rotate / toggle
        // triggers, and its spawn triggers in turn (with their delays, 4 deep)
        std::function<void(int, double, bool, int, int)> spawnChain = [&](int groupID, double x, bool armed, int when, int depth) {
            auto* group = layer->getGroup(groupID);
            if (!group || depth > 4) {
                return;
            }
            int seen = 0;
            for (auto* object : CCArrayExt<GameObject*>(group)) {
                if (!object || ++seen > 600) {
                    break;
                }
                int id = object->m_objectID;
                if (id != 901 && id != 1346 && id != 1049 && id != 1268) {
                    continue;
                }
                auto* c = static_cast<EffectGameObject*>(object);
                if (!c->m_isSpawnTriggered) {
                    continue;
                }
                if (id == 1268) {
                    if (plainSpawn(c)) {
                        spawnChain(c->m_targetGroupID, x, armed, when + spawnSteps(c, tickLength), depth + 1);
                    }
                }
                else if (id == 1049) {
                    if (c->m_targetGroupID > 0) {
                        addMove(c, x, TriggerKind::Toggle, armed, when);
                    }
                }
                else if (plainTrigger(c, id == 1346)) {
                    addMove(c, x, id == 1346 ? TriggerKind::Rotate : TriggerKind::Move, armed, when);
                }
            }
        };
        // groups the real game will spawn shortly (a spawn trigger with a delay already fired)
        if (!pendingSpawns.empty()) {
            std::erase_if(pendingSpawns, [](PendingSpawn const& p) { return p.at < tickCount; });
            for (auto const& p : pendingSpawns) {
                spawnChain(p.group, 0.0, true, p.at - tickCount, 0);
            }
        }
        // the ones the ghost will set off
        auto it = std::lower_bound(levelTriggers.begin(), levelTriggers.end(), px - 2.0, [](auto const& m, double x) { return m.x < x; });
        for (; it != levelTriggers.end() && it->x <= px + reach; ++it) {
            if (it->trigger->m_activated) {
                continue;
            }
            if (it->kind == TriggerKind::Spawn) {
                if (!it->touch) {
                    spawnChain(it->trigger->m_targetGroupID, it->x, false, spawnSteps(it->trigger, tickLength), 0);
                }
            }
            else {
                addMove(it->trigger, it->x, it->kind, false, 0, it->touch, it->rect);
            }
        }
        buf.triggersAhead = static_cast<int>(scriptedMoves.size());
        if (scriptedMoves.empty()) {
            scriptedObjects.clear();
        }
        cache.layer = layer;
        cache.tick = tickCount;
        cache.px = px;
        cache.reach = reach;
        cache.moves = scriptedMoves;
        cache.objects = scriptedObjects;
    }

    // v2.85: a toggle in this prediction: off = the ghost stops colliding with the group, on =
    // switched-off objects in it collide again (see collisionCheckObjects)
    void simToggle(CCArray* group, bool on) {
        int seen = 0;
        for (auto* object : CCArrayExt<GameObject*>(group)) {
            if (!object || ++seen > 2000) {
                break;
            }
            if (!on) {
                buf.simDisabled.insert(object);
                continue;
            }
            buf.simDisabled.erase(object);
            if (object->m_isGroupDisabled && buf.simEnabled.size() < 256 &&
                std::find(buf.simEnabled.begin(), buf.simEnabled.end(), object) == buf.simEnabled.end()) {
                buf.simEnabled.push_back(object);
            }
        }
    }

    // the ghost reached x: triggers there go off (v2.85: after their spawn delay); a toggle
    // switches its group off (or on) for the rest of this prediction
    void fireScripted(double gx, CCRect const& ghostRect, int tick) {
        for (auto& m : scriptedMoves) {
            if (!m.armed && (m.touch ? ghostRect.intersectsRect(m.rect) : gx >= m.x)) {
                m.armed = true;
                m.fireStep = tick + m.delay;
            }
            if (m.kind == TriggerKind::Toggle && m.armed && !m.toggled && tick >= m.fireStep && m.group) {
                m.toggled = true;
                simToggle(m.group, m.on);
            }
        }
    }

    // where the triggered objects are `step` steps into the prediction
    void placeScripted(int step) {
        for (auto& o : scriptedObjects) {
            double x = o.mover ? o.object->m_positionX : o.x;
            double y = o.mover ? o.object->m_positionY : o.y;
            double spin = 0.0;
            bool any = false;
            for (int i : o.moves) {
                auto const& m = scriptedMoves[i];
                if (!m.armed || step <= m.fireStep) {
                    continue;
                }
                double f = (m.steps < 0.5 ? 1.0 : easeMove(m.easing, m.rate, (step - m.fireStep) / m.steps)) - m.from;
                if (f == 0.0) {
                    continue;
                }
                any = true;
                applyMove(m, f, x, y, spin);
            }
            if (!any && !o.placed) {
                continue;
            }
            // v2.84.5: a finished (or instant) move leaves it where it was last step: nothing to do
            if (o.placed && !o.mover && x == o.lastX && y == o.lastY && spin == o.lastSpin) {
                continue;
            }
            o.lastX = x;
            o.lastY = y;
            o.lastSpin = spin;
            o.object->setPosition({ static_cast<float>(x), static_cast<float>(y) });
            o.object->m_positionX = x;
            o.object->m_positionY = y;
            if (!o.mover && (spin != 0.0 || o.rotated)) {
                o.object->setRotationX(static_cast<float>(o.rotX + spin));
                o.object->setRotationY(static_cast<float>(o.rotY + spin));
                o.rotated = true;
            }
            o.object->setObjectRectDirty(true);
            o.object->m_isOrientedBoxDirty = true;
            o.placed = true;
        }
    }

    void restoreScripted() {
        for (auto& o : scriptedObjects) {
            if (o.placed && !o.mover) {
                o.object->setPosition(o.node);
                o.object->m_positionX = o.x;
                o.object->m_positionY = o.y;
                if (o.rotated) {
                    o.object->setRotationX(o.rotX);
                    o.object->setRotationY(o.rotY);
                }
                o.object->setObjectRectDirty(true);
                o.object->m_isOrientedBoxDirty = true;
            }
        }
        scriptedObjects.clear();
        scriptedMoves.clear();
    }
