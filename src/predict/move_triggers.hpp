//@ predict/move_triggers.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // v2.81 move triggers ahead: spikes, blocks and orbs that are still right now but that a
    // move trigger further along will start moving (the ORBIT case: the prediction saw them
    // frozen where they were). When the ghost passes such a trigger during a prediction, its
    // group starts moving the way the game moves it (offset, duration, easing), on top of
    // any motion it already has; everything is put back exactly afterwards. Only plain moves:
    // target mode, lock to player / camera and spawn / touch triggered moves are skipped.
    // v2.83: rotate triggers too (saws and arms that start spinning, around their center
    // group or each in place), and toggle triggers that switch a group off (the ghost stops
    // colliding with it, like a toggle orb since v2.75).
    // v2.85: spawn triggers too, followed through their delays to the move / rotate / toggle
    // triggers they set off (and spawn triggers they set off, 4 deep), and toggles that switch
    // a group on (the ghost starts colliding with it).
    enum class TriggerKind { Move, Rotate, Toggle, Spawn };
    struct LevelTrigger {
        EffectGameObject* trigger;
        double x;
        TriggerKind kind;
        bool touch = false;  // v3.0 test 31: a touch trigger (goes off when the player touches it)
        CCRect rect {};
    };
    std::vector<LevelTrigger> levelTriggers;  // the level's position-activated triggers we predict, by x
    GJBaseGameLayer* levelTriggersLayer = nullptr;
    unsigned levelTriggersCount = 0;
    // move / rotate triggers the real game fired (any kind, spawn / touch too) and the step
    // they fired, so ones still in progress at your click follow their real eased path
    std::unordered_map<EffectGameObject*, int> firedMoves;

    struct ScriptedMove {
        TriggerKind kind = TriggerKind::Move;
        double x = 0.0;
        int fireStep = -1;  // prediction step the ghost reached the trigger (-1 = not yet)
        double dx = 0.0, dy = 0.0;  // move
        double angle = 0.0;         // rotate: degrees, clockwise
        bool hasCenter = false;
        double cx = 0.0, cy = 0.0;
        bool lockRot = false;
        double steps = 0.0;  // how many physics steps it lasts
        int easing = 0;
        double rate = 0.0;
        double from = 0.0;  // how far along it already was when the prediction started
        CCArray* group = nullptr;  // toggle: the group it switches off (or on)
        bool on = false;           // toggle: switches it on
        bool armed = false;        // v2.85: fired (fireStep is when it acts, maybe after a spawn delay)
        bool touch = false;        // v3.0 test 31: goes off when the ghost touches `rect`, not at x
        CCRect rect {};
        int delay = 0;             // v2.85: steps from the ghost reaching x to it acting (spawn delays)
        bool toggled = false;
    };
    struct ScriptedObject {
        GameObject* object;
        double x, y;     // m_positionX / Y before the prediction
        CCPoint node;    // and the node position, put back as it was
        float rotX, rotY;
        bool mover;      // also moved by placeMovers, which sets its base position every step
        bool placed = false;
        bool rotated = false;
        std::vector<int> moves;
        double lastX = 0.0, lastY = 0.0, lastSpin = 0.0;  // v2.84.5: where it was put last step
    };
    std::vector<ScriptedMove> scriptedMoves;
    std::vector<ScriptedObject> scriptedObjects;
    void forgetTriggerChecks();
    constexpr int SCRIPTED_TRIGGER_LIMIT = 48;
    // v3.0 test 31: autoplay's route looks up to 10 s ahead: room for many more
    int scriptedTriggerLimit() {
        return buf.offscreenFill ? 400 : SCRIPTED_TRIGGER_LIMIT;
    }
    // v2.85: groups the real game spawns after a delay, and the tick that happens
    struct PendingSpawn {
        int group;
        int at;
    };
    std::vector<PendingSpawn> pendingSpawns;
    constexpr int SCRIPTED_OBJECT_LIMIT = 160;
    size_t scriptedObjectLimit() {
        return buf.offscreenFill ? 1500 : SCRIPTED_OBJECT_LIMIT;
    }
    // v2.84.4: what collectScripted found, reused by every run of the same click (it used to
    // scan the trigger groups again for every run: on ORBIT that slowed each run so much that
    // orb searches stopped after 0-2 timings)
    struct ScriptedCache {
        GJBaseGameLayer* layer = nullptr;
        int tick = -1;
        double px = 0.0, reach = 0.0;
        std::vector<ScriptedMove> moves;
        std::vector<ScriptedObject> objects;
    } scriptedCache;

    void forgetMoveTriggers() {
        scriptedCache = {};
        pendingSpawns.clear();
        forgetTriggerChecks();  // v2.84: a new level starts the self-check over
        firedMoves.clear();
        levelTriggers.clear();
        levelTriggersLayer = nullptr;
        levelTriggersCount = 0;
    }

    // the game's easing curves (cocos2d's), by GD's easing number
    double easeMove(int type, double rate, double t) {
        constexpr double PI = 3.141592653589793;
        t = std::clamp(t, 0.0, 1.0);
        if (t <= 0.0 || t >= 1.0) {
            return t;
        }
        rate = rate > 0.0 ? rate : 2.0;
        auto bounce = [](double u) {
            if (u < 1.0 / 2.75) {
                return 7.5625 * u * u;
            }
            if (u < 2.0 / 2.75) {
                u -= 1.5 / 2.75;
                return 7.5625 * u * u + 0.75;
            }
            if (u < 2.5 / 2.75) {
                u -= 2.25 / 2.75;
                return 7.5625 * u * u + 0.9375;
            }
            u -= 2.625 / 2.75;
            return 7.5625 * u * u + 0.984375;
        };
        double period = rate > 0.0 && rate < 2.0 ? rate : 0.3;
        double s = period / 4.0;
        constexpr double BACK = 1.70158;
        switch (type) {
            case 1: {  // ease in out
                double u = t * 2.0;
                return u < 1.0 ? 0.5 * std::pow(u, rate) : 1.0 - 0.5 * std::pow(2.0 - u, rate);
            }
            case 2: return std::pow(t, rate);
            case 3: return std::pow(t, 1.0 / rate);
            case 4: {  // elastic in out
                double u = t * 2.0 - 1.0;
                return u < 0.0 ? -0.5 * std::pow(2.0, 10.0 * u) * std::sin((u - s) * 2.0 * PI / period)
                               : std::pow(2.0, -10.0 * u) * std::sin((u - s) * 2.0 * PI / period) * 0.5 + 1.0;
            }
            case 5: {
                double u = t - 1.0;
                return -std::pow(2.0, 10.0 * u) * std::sin((u - s) * 2.0 * PI / period);
            }
            case 6: return std::pow(2.0, -10.0 * t) * std::sin((t - s) * 2.0 * PI / period) + 1.0;
            case 7: return t < 0.5 ? (1.0 - bounce(1.0 - t * 2.0)) * 0.5 : bounce(t * 2.0 - 1.0) * 0.5 + 0.5;
            case 8: return 1.0 - bounce(1.0 - t);
            case 9: return bounce(t);
            case 10: {
                double u = t * 2.0;
                return u < 1.0 ? 0.5 * std::pow(2.0, 10.0 * (u - 1.0)) : 0.5 * (2.0 - std::pow(2.0, -10.0 * (u - 1.0)));
            }
            case 11: return std::pow(2.0, 10.0 * (t - 1.0));
            case 12: return 1.0 - std::pow(2.0, -10.0 * t);
            case 13: return -0.5 * (std::cos(PI * t) - 1.0);
            case 14: return 1.0 - std::cos(t * PI / 2.0);
            case 15: return std::sin(t * PI / 2.0);
            case 16: {
                double o = BACK * 1.525;
                double u = t * 2.0;
                if (u < 1.0) {
                    return u * u * ((o + 1.0) * u - o) / 2.0;
                }
                u -= 2.0;
                return u * u * ((o + 1.0) * u + o) / 2.0 + 1.0;
            }
            case 17: return t * t * ((BACK + 1.0) * t - BACK);
            case 18: {
                double u = t - 1.0;
                return u * u * ((BACK + 1.0) * u + BACK) + 1.0;
            }
            default: return t;
        }
    }

    bool plainTrigger(EffectGameObject* t, bool rotate) {
        if (t->m_targetGroupID <= 0 || t->m_isDynamicMode) {
            return false;
        }
        if (rotate) {
            return t->m_rotationTargetID <= 0 && (t->m_rotationDegrees != 0.f || t->m_times360 != 0);
        }
        if (t->m_lockToPlayerX || t->m_lockToPlayerY || t->m_lockToCameraX || t->m_lockToCameraY) {
            return false;
        }
        if (t->m_useMoveTarget) {
            return t->m_targetModCenterID > 0;  // v3.0 test 31: target mode (to another group's spot)
        }
        return t->m_moveOffset.x != 0.f || t->m_moveOffset.y != 0.f;
    }

    // v2.85: a spawn trigger the prediction can follow: no random delay, no remapped groups, not ordered
    bool plainSpawn(EffectGameObject* t) {
        auto* s = static_cast<SpawnTriggerGameObject*>(t);
        return t->m_targetGroupID > 0 && s->m_delayRange <= 0.f && s->m_remapObjects.empty() && !t->m_spawnOrdered;
    }

    int spawnSteps(EffectGameObject* t, float tickLength) {
        return static_cast<int>(std::lround(std::max(0.f, static_cast<SpawnTriggerGameObject*>(t)->m_spawnDelay) * 60.0 / tickLength));
    }

    // the level's move / rotate / toggle / spawn triggers, found once per level (and again if
    // objects were added)
    void scanMoveTriggers(GJBaseGameLayer* layer) {
        unsigned count = layer->m_objects ? layer->m_objects->count() : 0;
        if (layer == levelTriggersLayer && count == levelTriggersCount) {
            return;
        }
        levelTriggersLayer = layer;
        levelTriggersCount = count;
        levelTriggers.clear();
        if (!layer->m_objects) {
            return;
        }
        for (auto* object : CCArrayExt<GameObject*>(layer->m_objects)) {
            if (!object) {
                continue;
            }
            int id = object->m_objectID;
            if (id != 901 && id != 1346 && id != 1049 && id != 1268) {
                continue;
            }
            auto* t = static_cast<EffectGameObject*>(object);
            if (t->m_isSpawnTriggered) {
                continue;
            }
            bool touch = t->m_isTouchTriggered;
            CCRect rect = touch ? t->getObjectRect() : CCRect {};
            if (id == 1049) {
                if (t->m_targetGroupID > 0) {
                    levelTriggers.push_back({ t, t->m_positionX, TriggerKind::Toggle, touch, rect });  // v2.85: on too
                }
                continue;
            }
            if (id == 1268) {
                if (plainSpawn(t)) {
                    levelTriggers.push_back({ t, t->m_positionX, TriggerKind::Spawn, touch, rect });
                }
                continue;
            }
            bool rotate = id == 1346;
            if (plainTrigger(t, rotate)) {
                levelTriggers.push_back({ t, t->m_positionX, rotate ? TriggerKind::Rotate : TriggerKind::Move, touch, rect });
            }
        }
        std::sort(levelTriggers.begin(), levelTriggers.end(), [](auto const& a, auto const& b) { return a.x < b.x; });
    }

    // a move / rotate trigger's motion: offset or angle (and center), duration, easing
    // a group's group parent (set in the editor), or nullptr
    GameObject* groupParentOf(GJBaseGameLayer* layer, int group) {
        if (group <= 0 || !layer->m_parentGroupsDict) {
            return nullptr;
        }
        return typeinfo_cast<GameObject*>(layer->m_parentGroupsDict->objectForKey(group));
    }

    void fillMove(GJBaseGameLayer* layer, EffectGameObject* t, TriggerKind kind, float tickLength, ScriptedMove& m) {
        m.kind = kind;
        m.steps = t->m_duration * 60.0 / tickLength;
        m.easing = static_cast<int>(t->m_easingType);
        m.rate = t->m_easingRate;
        if (kind == TriggerKind::Move) {
            m.dx = t->m_moveOffset.x;
            m.dy = t->m_moveOffset.y;
            if (t->m_useMoveTarget) {
                // v3.0 test 31: target mode: the group goes to where the target group is (its group
                // parent, or its first object), measured from the center group (or the group itself)
                auto spot = [&](int group) -> GameObject* {
                    if (group <= 0) {
                        return nullptr;
                    }
                    if (auto* parent = groupParentOf(layer, group)) {
                        return parent;
                    }
                    auto* objects = layer->getGroup(group);
                    return objects && objects->count() > 0 ? static_cast<GameObject*>(objects->objectAtIndex(0)) : nullptr;
                };
                auto* target = spot(t->m_targetModCenterID);
                auto* from = spot(t->m_centerGroupID > 0 ? t->m_centerGroupID : t->m_targetGroupID);
                m.dx = m.dy = 0.0;
                if (target && from) {
                    if (t->m_moveTargetMode != MoveTargetType::YOnly) {
                        m.dx = target->m_positionX - from->m_positionX;
                    }
                    if (t->m_moveTargetMode != MoveTargetType::XOnly) {
                        m.dy = target->m_positionY - from->m_positionY;
                    }
                }
            }
            return;
        }
        m.angle = t->m_rotationDegrees + 360.0 * t->m_times360;
        m.lockRot = t->m_lockObjectRotation;
        // v2.84.5: the pivot is the center group's group parent (or its first object); with no
        // center group, the target group's group parent, and only without one does each object
        // turn in place (ORBIT's saws orbit their group parent: they were predicted spinning in place)
        GameObject* pivot = nullptr;
        if (t->m_centerGroupID > 0) {
            pivot = groupParentOf(layer, t->m_centerGroupID);
            if (!pivot) {
                auto* center = layer->getGroup(t->m_centerGroupID);
                if (center && center->count() > 0) {
                    pivot = static_cast<GameObject*>(center->objectAtIndex(0));
                }
            }
        }
        else {
            pivot = groupParentOf(layer, t->m_targetGroupID);
        }
        if (pivot) {
            m.hasCenter = true;
            m.cx = pivot->m_positionX;
            m.cy = pivot->m_positionY;
        }
    }

    // fraction f of move m applied to a position (and an object's own turn)
    void applyMove(ScriptedMove const& m, double f, double& x, double& y, double& spin) {
        if (m.kind == TriggerKind::Move) {
            x += m.dx * f;
            y += m.dy * f;
            return;
        }
        if (m.kind != TriggerKind::Rotate) {
            return;
        }
        double a = m.angle * f;
        if (m.hasCenter) {
            // clockwise around the center (y up)
            double th = -a * 3.141592653589793 / 180.0;
            double rx = x - m.cx, ry = y - m.cy;
            x = m.cx + rx * std::cos(th) - ry * std::sin(th);
            y = m.cy + rx * std::sin(th) + ry * std::cos(th);
        }
        if (!m.lockRot) {
            spin += a;
        }
    }
