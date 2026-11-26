//@ route/object_index.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    void buildObjIndex(PlayLayer* layer) {
        buf.objIndex.clear();
        if (!layer->m_objects) {
            return;
        }
        uint64_t keep = COLLIDING_TYPES | INTERACT_TYPES | STOP_TYPES | SIZE_TYPES;
        for (auto* o : CCArrayExt<GameObject*>(layer->m_objects)) {
            if (!o) {
                continue;
            }
            auto type = static_cast<unsigned>(o->m_objectType);
            uint64_t bit = type < 64 ? (1ull << type) : 0;
            if (!(keep & bit) && speedPortalValue(o->m_objectID) <= 0.f) {
                continue;
            }
            buf.objIndex[static_cast<int>(std::floor(o->m_positionX / 64.f))].push_back(o);
        }
    }
