//@ predict/trigger_check.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // v2.84 trigger self-check: when a real move / rotate trigger fires, one of its objects
    // near you is watched for up to half a second, and where it really ends up is compared with
    // where the prediction would have put it. If this level's triggers keep coming out wrong,
    // trigger prediction turns itself off for the level.
    // v2.84.2: the prediction for the watched object is the combined motion of EVERY move /
    // rotate trigger that acted on any of its groups in that time (ORBIT moves almost every
    // block with several triggers at once, so v2.84.1 skipped every check). Only when a trigger
    // the prediction can't model acted on it (follow, keyframes, target / lock moves...) is
    // the check left out.
    struct TriggerCheck {
        GameObject* object;
        int fired;
        int at;
        double x, y, rot;
    };
    std::vector<TriggerCheck> triggerChecks;
    int checksGood = 0, checksBad = 0, checksUnclear = 0;
    bool triggersDistrusted = false;
    int attemptStartTick = 0;  // v2.84.3
    // every trigger with a target group the real game fired lately, and when
    struct TriggerFire {
        EffectGameObject* trigger;
        int tick;
        ScriptedMove move;  // v2.84.5: filled when it fired (the pivot where it was then)
    };
    std::vector<TriggerFire> triggerFires;

    void noteTriggerFire(GJBaseGameLayer* layer, EffectGameObject* t) {
        if (t->m_targetGroupID <= 0) {
            return;
        }
        if (triggerFires.size() > 1024) {
            triggerFires.erase(triggerFires.begin(), triggerFires.begin() + 512);
        }
        ScriptedMove m;
        int id = t->m_objectID;
        if (layer && (id == 901 || id == 1346)) {
            float tickLength = buf.tickLength > 0.001f && buf.tickLength < 2.f ? buf.tickLength : TICK;
            fillMove(layer, t, id == 1346 ? TriggerKind::Rotate : TriggerKind::Move, tickLength, m);
        }
        triggerFires.push_back({ t, tickCount, m });
    }

    // triggers that move objects in ways the prediction doesn't model
    bool unmodeledMover(int id) {
        switch (id) {
            case 1347:  // follow
            case 1814:  // follow player Y
            case 2067:  // scale
            case 3016:  // advanced follow
            case 3033:  // keyframe animation
            case 3006: case 3007: case 3008: case 3009: case 3010:  // area move / rotate / scale / fade / tint
                return true;
            default:
                return false;
        }
    }

    bool inGroups(GameObject* object, int group) {
        auto* groups = object->m_groups;
        int count = std::clamp<int>(object->m_groupCount, 0, 10);
        if (!groups || group <= 0) {
            return false;
        }
        for (int i = 0; i < count; ++i) {
            if ((*groups)[i] == group) {
                return true;
            }
        }
        return false;
    }

    void startTriggerCheck(GJBaseGameLayer* layer, EffectGameObject* t) {
        // v2.84.3: checks keep running while trigger prediction is off, so it can come back on;
        // triggers in the first quarter second of an attempt (level setup) aren't checked
        if (!buf.predictTriggers || triggerChecks.size() >= 16 || !layer || !layer->m_player1 || tickCount - attemptStartTick < st(60)) {
            return;
        }
        bool rotate = t->m_objectID == 1346;
        if (!plainTrigger(t, rotate) || t->m_duration * 60.f < TICK) {
            return;
        }
        auto* group = layer->getGroup(t->m_targetGroupID);
        if (!group) {
            return;
        }
        float tickLength = buf.tickLength > 0.001f && buf.tickLength < 2.f ? buf.tickLength : TICK;
        double px = layer->m_player1->getPositionX();
        for (auto* object : CCArrayExt<GameObject*>(group)) {
            if (!object || object == t || object->m_isDecoration || object->m_objectType == GameObjectType::Decoration ||
                object->m_positionX < px - 150.0 || object->m_positionX > px + 700.0) {
                continue;
            }
            for (auto const& c : triggerChecks) {
                if (c.object == object) {
                    return;  // already watched
                }
            }
            int span = std::min(static_cast<int>(std::lround(t->m_duration * 60.0 / tickLength)), st(120));
            triggerChecks.push_back({ object, tickCount, tickCount + std::max(span, 1), object->m_positionX, object->m_positionY,
                static_cast<double>(object->getRotationX()) });
            return;
        }
    }

    void forgetTriggerChecks() {
        triggerChecks.clear();
        triggerFires.clear();
        checksGood = checksBad = checksUnclear = 0;
        triggersDistrusted = false;
    }

    // where the prediction says the object is now: every modeled trigger on its groups, each
    // from when the check started (or it fired) to now. nullptr = predicted, else why not.
    char const* predictCheck(GJBaseGameLayer* layer, TriggerCheck const& c, double& x, double& y, double& spin, int& used,
        std::string* detail = nullptr) {
        float tickLength = buf.tickLength > 0.001f && buf.tickLength < 2.f ? buf.tickLength : TICK;
        x = c.x;
        y = c.y;
        spin = 0.0;
        used = 0;
        for (auto const& f : triggerFires) {
            if (f.tick > c.at || !inGroups(c.object, f.trigger->m_targetGroupID)) {
                continue;
            }
            int id = f.trigger->m_objectID;
            if (unmodeledMover(id)) {
                if (f.tick >= c.fired - st(240)) {
                    return "a follow / keyframe / area trigger moves it";
                }
                continue;
            }
            if (id != 901 && id != 1346) {
                continue;
            }
            ScriptedMove m = f.move;
            bool rotate = id == 1346;
            int start = std::max(c.fired, f.tick);
            if (m.steps >= 0.5 && f.tick + m.steps <= c.fired) {
                continue;  // finished before the check started
            }
            if (!plainTrigger(f.trigger, rotate)) {
                return "a target / lock-to-player move moves it";
            }
            double done = m.steps < 0.5 ? 1.0 : easeMove(m.easing, m.rate, (c.at - f.tick) / m.steps);
            double before = m.steps < 0.5 ? (f.tick < c.fired ? 1.0 : 0.0) : easeMove(m.easing, m.rate, (start - f.tick) / m.steps);
            applyMove(m, done - before, x, y, spin);
            ++used;
            if (detail) {
                // v2.84.4: what each trigger was, for misses: id, offset or angle, duration, easing
                auto* t = f.trigger;
                *detail += rotate ? fmt::format(" | rotate {:g}deg{}{} {:.2f}s e{}/{:g}", m.angle, m.hasCenter ? " around center" : " in place",
                                        m.lockRot ? " locked" : "", t->m_duration, m.easing, m.rate)
                                  : fmt::format(" | move {:g},{:g}{}{} {:.2f}s e{}/{:g}", m.dx, m.dy, t->m_smallStep ? " smallstep" : "",
                                        t->m_isSilent ? " silent" : "", t->m_duration, m.easing, m.rate);
                *detail += fmt::format(" from t{}", f.tick - c.fired);
            }
        }
        return nullptr;
    }

    void runTriggerChecks(GJBaseGameLayer* layer) {
        for (auto it = triggerChecks.begin(); it != triggerChecks.end();) {
            auto const& c = *it;
            if (tickCount < c.fired) {
                it = triggerChecks.erase(it);
                continue;
            }
            if (tickCount < c.at) {
                ++it;
                continue;
            }
            double x, y, spin;
            int used;
            std::string detail;
            char const* unclear = predictCheck(layer, c, x, y, spin, used, buf.explain ? &detail : nullptr);
            double moved = std::hypot(c.object->m_positionX - c.x, c.object->m_positionY - c.y);
            double turned = std::abs(std::remainder(c.object->getRotationX() - c.rot, 360.0));
            if (!unclear && moved < 0.5 && turned < 0.5) {
                unclear = "it didn't move";  // not updated yet, or a stop trigger
            }
            if (unclear) {
                ++checksUnclear;
                if (buf.explain && checksUnclear % 10 == 1) {
                    say("trigger check: unclear ({}), {} unclear so far", unclear, checksUnclear);
                }
                it = triggerChecks.erase(it);
                continue;
            }
            double err = std::hypot(c.object->m_positionX - x, c.object->m_positionY - y);
            double tol = std::max(3.0, std::hypot(x - c.x, y - c.y) * 0.1);
            double turnErr = std::abs(std::remainder(c.object->getRotationX() - (c.rot + spin), 360.0));
            bool ok = err <= tol && turnErr <= std::max(5.0, std::abs(spin) * 0.1);
            ok ? ++checksGood : ++checksBad;
            if (buf.explain) {
                say("trigger check: {} the prediction ({} trigger{}, off by {:.1f}u {:.0f} deg | {} hit / {} missed / {} unclear)",
                    ok ? "matches" : "DOESN'T match", used, used == 1 ? "" : "s", err, turnErr, checksGood, checksBad, checksUnclear);
                if (!ok) {
                    // v2.84.4: expected vs real movement and the triggers involved
                    say("  obj {} expected moved {:.1f},{:.1f} turned {:.0f} | really moved {:.1f},{:.1f} turned {:.0f} over {}t{}",
                        c.object->m_objectID, x - c.x, y - c.y, spin, c.object->m_positionX - c.x, c.object->m_positionY - c.y,
                        std::remainder(c.object->getRotationX() - c.rot, 360.0), c.at - c.fired, detail);
                }
            }
            // v2.84.3: decided on 10+ checks, and it comes back on if hits catch up (ORBIT went
            // off at 1 hit / 5 missed from level-start triggers, then ended at 9 hit / 8 missed)
            bool wrong = checksGood + checksBad >= 10 && checksBad > 2 * checksGood;
            if (wrong != triggersDistrusted) {
                triggersDistrusted = wrong;
                if (wrong) {
                    say("trigger check: {} of {} trigger predictions were wrong on this level, trigger prediction is off here", checksBad,
                        checksBad + checksGood);
                }
                else {
                    say("trigger check: {} of {} wrong now, trigger prediction is back on", checksBad, checksBad + checksGood);
                }
            }
            it = triggerChecks.erase(it);
        }
    }
