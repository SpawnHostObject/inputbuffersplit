# Source layout

`main.cpp` is the only file the build compiles (`file(GLOB ... src/*.cpp)`). It includes every other file
in the same order the code had in the old single `main.cpp`. The pieces are plain text includes, not separate
translation units, so nothing about linkage, the anonymous namespace or declaration order changed.

Rules when editing:
- A file can only use things defined in files included **before** it in `main.cpp` (or declared by a forward declaration).
- To add a file, create it in the matching folder and add one `#include "folder/file.hpp"` line in `main.cpp` at the right spot.
- Lines starting with `//@ ` at the top of each file are a banner and can be deleted or kept.
- `ui/buffer_menu/*.inl` are member functions of `BufferMenu`; they are included inside the class body.

| Folder | Contents |
|---|---|
| core | constants, shared state (`buf`), settings, stage handling, log, startup (`$on_mod`) |
| ghost | the hidden ghost player: creation, state copy, stepping, `simulate()` |
| predict | portals, moving objects, move/rotate/toggle/spawn trigger prediction, trigger self-check |
| scoring | per-mode scoring and delay searches (wave, ship, UFO, robot, spider, orbs, cube) |
| decision | `intercept()` (the per-click decision), simulate with orbs, click timer, auto-release, auto-click, input delivery |
| input | hold statistics, input queue, keybinds |
| memory | per-level memory and per-level profiles |
| stats | session statistics |
| debug | drift check, path view, live path, run check |
| autoplay | autoplay state, macros (parser, loading, online, playback, fixes, guard), global brain, short planner |
| route | long-range route planner: snapshots, sprint search, beam search, pathfinder, per-tick driver |
| net | online players, account linking, update check |
| ui | HUD labels, heatmap, pause-menu button, icon, Choose Macro popup, `buffer_menu/` (the main window) |
| hooks | one file per hooked class (`$modify`) |
