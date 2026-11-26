//@ predict/moving_objects.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // v2.4 moving-object guard: objects the ghost collided with are watched every tick,
    // and any that moved recently make the decision "don't buffer" (the ghost sees the
    // level frozen at the click, so it tests them in the wrong place).
    void watchObject(GameObject* object) {
        auto [it, added] = watched.try_emplace(object, Watched { object->m_positionX, object->m_positionY, -MOVER_TICKS });
        if (added) {
            it->second.rot = object->getRotation();
        }
        if (!added && tickCount - it->second.movedTick < st(MOVER_TICKS)) {
            buf.sawMover = true;
        }
    }

    // v2.41: objects that moved on the last step, so predictions don't have to search the
    // whole watch list for them.
    std::vector<GameObject*> movingNow;

    void updateWatched() {
        if (watched.size() > WATCH_LIMIT) {
            watched.clear();
        }
        movingNow.clear();
        // v2.41: objects far from you (passed long ago, or far ahead) are dropped instead of
        // being checked every step; predictions never reach further than ~300 units.
        double px = 0.0;
        bool prune = false;
        if (auto* layer = GameManager::get()->m_playLayer; layer && layer->m_player1) {
            px = layer->m_player1->getPosition().x;
            prune = true;
        }
        for (auto it = watched.begin(); it != watched.end();) {
            auto* object = it->first;
            auto& w = it->second;
            if (prune && std::abs(object->m_positionX - px) > 800.0) {
                it = watched.erase(it);
                continue;
            }
            double rot = object->getRotation();
            if (object->m_positionX != w.x || object->m_positionY != w.y || rot != w.rot) {
                // called every step, so the change is the movement per step
                bool consecutive = tickCount - w.movedTick == 1;
                w.pvx = consecutive ? w.vx : 0.0;
                w.pvy = consecutive ? w.vy : 0.0;
                w.vx = object->m_positionX - w.x;
                w.vy = object->m_positionY - w.y;
                double dr = rot - w.rot;
                w.vrot = std::remainder(dr, 360.0);
                w.x = object->m_positionX;
                w.y = object->m_positionY;
                w.rot = rot;
                w.movedTick = tickCount;
                movingNow.push_back(object);
            }
            else if (tickCount - w.movedTick > 1) {
                w.vx = w.vy = w.pvx = w.pvy = w.vrot = 0.0;  // stopped
            }
            ++it;
        }
    }

    // v2.27 moving-object prediction. Objects near your path that are moving right now are
    // moved along their current motion during a prediction (where they'll be k steps
    // later), and put back exactly afterwards. Straight-line movement only: easing,
    // rotation and toggles still aren't predicted.
    // v2.68: also curved paths (rotate triggers move objects around a center, Orbit's saws):
    // when the direction of motion turned between the last two steps, it keeps turning by the
    // same angle each step, and objects that spin keep spinning. Still put back exactly after.
    struct Mover {
        GameObject* object;
        double x, y, vx, vy;
        double turn = 0.0;  // radians the motion direction turns per step (0 = straight)
        double rotX = 0.0, rotY = 0.0, vrot = 0.0;  // both axes kept as they were (skewed objects)
        double ax = 0.0, ay = 0.0;  // v2.75: speeding up / slowing down (easing), per step
        int accelSteps = 0;         // v2.75: how long the easing is followed (until it would stop / over 3x speed)
    };
    std::vector<Mover> movers;

    void collectMovers() {
        movers.clear();
        if (!buf.predictMovers) {
            return;
        }
        for (auto* object : movingNow) {
            auto it = watched.find(object);
            if (it == watched.end()) {
                continue;
            }
            auto const& w = it->second;
            if ((w.vx != 0.0 || w.vy != 0.0 || w.vrot != 0.0) && tickCount - w.movedTick <= 1) {
                Mover m { object, object->m_positionX, object->m_positionY, w.vx, w.vy };
                double speed = std::hypot(w.vx, w.vy);
                double prev = std::hypot(w.pvx, w.pvy);
                if (speed > 1e-6 && prev > 1e-6) {
                    double turn = std::remainder(std::atan2(w.vy, w.vx) - std::atan2(w.pvy, w.pvx), 6.283185307179586);
                    double ratio = speed / prev;
                    // a steady turn at a steady speed; anything wilder stays a straight line
                    if (std::abs(turn) > 1e-4 && std::abs(turn) < 0.5 && ratio > 0.8 && ratio < 1.25) {
                        m.turn = turn;
                    }
                    // v2.75: straight, but faster or slower than the step before: eased movement
                    else if (std::abs(turn) <= 1e-4 && std::abs(ratio - 1.0) > 1e-3 && ratio > 0.5 && ratio < 2.0) {
                        m.ax = w.vx - w.pvx;
                        m.ay = w.vy - w.pvy;
                        double along = (m.ax * w.vx + m.ay * w.vy) / speed;  // + speeding up, - slowing down
                        double steps = along < 0.0 ? speed / -along : 2.0 * speed / along;
                        m.accelSteps = static_cast<int>(std::min(steps, 1000.0));
                    }
                }
                m.rotX = object->getRotationX();
                m.rotY = object->getRotationY();
                m.vrot = w.vrot;
                movers.push_back(m);
            }
        }
    }

    void placeMovers(int steps) {
        for (auto const& m : movers) {
            double x = m.x + m.vx * steps;
            double y = m.y + m.vy * steps;
            if (m.accelSteps > 0 && steps > 0) {
                // eased: the speed keeps changing the same way, then stays (stopped or at 3x)
                int n = std::min(steps, m.accelSteps);
                double grow = 0.5 * n * (n + 1);
                x = m.x + m.vx * steps + m.ax * (grow + static_cast<double>(n) * (steps - n));
                y = m.y + m.vy * steps + m.ay * (grow + static_cast<double>(n) * (steps - n));
            }
            if (m.turn != 0.0 && steps > 0) {
                // sum of the step vector turned by `turn` once more every step
                std::complex<double> v { m.vx, m.vy };
                auto rho = std::polar(1.0, m.turn);
                auto moved = v * rho * (1.0 - std::pow(rho, steps)) / (1.0 - rho);
                x = m.x + moved.real();
                y = m.y + moved.imag();
            }
            if (m.vrot != 0.0) {
                double spun = steps == 0 ? 0.0 : m.vrot * steps;
                m.object->setRotationX(static_cast<float>(m.rotX + spun));
                m.object->setRotationY(static_cast<float>(m.rotY + spun));
            }
            m.object->setPosition({ static_cast<float>(x), static_cast<float>(y) });
            m.object->m_positionX = x;
            m.object->m_positionY = y;
            m.object->setObjectRectDirty(true);
            m.object->m_isOrientedBoxDirty = true;
        }
    }
