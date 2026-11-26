//@ hooks/game_object.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
//@ 
//@ 
//@ 
//@ 
//@ 
class $modify(BufGameObject, GameObject) {
    static void onModify(auto& self) {
        (void)self.setHookPriority("GameObject::playShineEffect", HOOK_PRIORITY);
    }

    void playShineEffect() {
        if (buf.simulating) {
            return;
        }
        GameObject::playShineEffect();
    }

    // v2.78.3: visual only, never from a ghost
    void playDestroyObjectAnim(GJBaseGameLayer* layer) {
        if (buf.simulating) {
            return;
        }
        GameObject::playDestroyObjectAnim(layer);
    }
};
