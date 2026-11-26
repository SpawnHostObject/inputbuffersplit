//@ hooks/editor_layer.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
//@ 
class $modify(BufEditorLayer, LevelEditorLayer) {
    bool init(GJGameLevel* level, bool noUI) {
        bool ok = LevelEditorLayer::init(level, noUI);
        dropGhosts();
        return ok;
    }
};
