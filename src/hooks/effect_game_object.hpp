//@ hooks/effect_game_object.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
//@ 
//@ 
//@ 
//@ 
class $modify(BufEffectGameObject, EffectGameObject) {
    static void onModify(auto& self) {
        (void)self.setHookPriority("EffectGameObject::triggerObject", HOOK_PRIORITY);
    }

    void triggerObject(GJBaseGameLayer* layer, int uniqueID, gd::vector<int> const* remapKeys) {
        if (buf.simulating) {
            return;
        }
        if (buf.predictTriggers) {
            noteTriggerFire(layer, this);  // v2.84.2: any trigger with a target group, for the self-check
        }
        // v2.81: when a move (v2.83: or rotation) started, so a prediction can follow the rest of it
        if ((m_objectID == 901 || m_objectID == 1346) && buf.predictTriggers && m_duration > 0.f && firedMoves.size() < 256) {
            firedMoves[this] = tickCount;
            startTriggerCheck(layer, this);  // v2.84
        }
        EffectGameObject::triggerObject(layer, uniqueID, remapKeys);
    }

    // v2.78.3: visual only, never from a ghost
    void playTriggerEffect() {
        if (buf.simulating) {
            return;
        }
        EffectGameObject::playTriggerEffect();
    }

    // v2.84.6: a pad's own animation (it played when a ghost touched the pad, before you did:
    // seen after gravity changes)
    void animationTriggered() {
        if (buf.simulating) {
            return;
        }
        EffectGameObject::animationTriggered();
    }
};
