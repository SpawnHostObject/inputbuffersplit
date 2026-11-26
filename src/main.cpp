// Click Before Frames v2.9 (cube-style wave, wave auto-release, orb alignment, full ghost state, drift check, in-game console on F9)
//
// When you click (or release, in wave / robot), a hidden "ghost" player is simulated
// ahead with your input. If the ghost dies, the input is retried with 1..N ticks of
// delay and, if a delayed version clearly survives longer, your input is held back
// and delivered that many physics ticks later. 1 tick = 1/240 s = 0.25 frames @ 60fps.
//
// v2 changes:
//  - Bigger windows: F6 stages up to 32 ticks (8 frames) plus a CUSTOM stage (settings).
//  - Smart prediction: ghost uses orbs, pads, gravity portals and speed portals; mode /
//    size / dual / mirror / teleport portals end the prediction as "safe".
//  - Input queue: every input arriving during a delay is shifted by the same amount,
//    so taps and hold lengths are preserved (no more swallowed clicks / 1-tick taps).
//  - Time budget per click so big windows don't hitch.
//  - Fixes: ghost lifetime, init using `this`, time-warp dt, F6 only when GD focused,
//    dead flipGravity hook priority, optional console, in-game console (stage + live log, hidden by default).

#include "core/includes.hpp"

using namespace geode::prelude;

namespace {
#include "core/constants.hpp"

#include "core/types.hpp"

#include "core/state.hpp"

#include "core/timing.hpp"

#include "core/tracking_types.hpp"

#include "core/log.hpp"

#include "ghost/ghost_identity.hpp"

#include "predict/portals.hpp"

#include "core/settings.hpp"

#include "core/click_budget.hpp"

#include "core/mode_config.hpp"

#include "core/stage.hpp"

#include "ui/hud_labels.hpp"

#include "core/export_log.hpp"

#include "ghost/ghost_lifecycle.hpp"

#include "core/button_io.hpp"

#include "ghost/copy_state.hpp"

#include "input/hold_stats.hpp"

#include "predict/moving_objects.hpp"

#include "predict/move_triggers.hpp"

#include "predict/trigger_check.hpp"

#include "predict/scripted_triggers.hpp"

#include "debug/drift_check.hpp"

#include "memory/level_memory.hpp"

#include "ghost/object_snapshot.hpp"

#include "ghost/ghost_step.hpp"

#include "ghost/simulate.hpp"

#include "decision/click_timer.hpp"

#include "decision/simulate_orbs.hpp"

#include "debug/path_view.hpp"

#include "scoring/horizons.hpp"

#include "scoring/next_click.hpp"

#include "scoring/orb_alignment.hpp"

#include "scoring/search_state.hpp"

#include "scoring/wave_score.hpp"

#include "scoring/robot_score.hpp"

#include "scoring/spider_score.hpp"

#include "scoring/search_core.hpp"

#include "scoring/ship_score.hpp"

#include "scoring/ufo_score.hpp"

#include "scoring/wave_search.hpp"

#include "scoring/delay_search.hpp"

#include "debug/live_path.hpp"

#include "input/queue.hpp"

#include "stats/session_stats.hpp"

#include "memory/level_profile.hpp"

#include "ui/death_heatmap.hpp"

#include "debug/run_check.hpp"

#include "stats/stats_text.hpp"

#include "decision/intercept.hpp"

#include "core/stage_cycle.hpp"
#include "net/online_players.hpp"

#include "net/account.hpp"

#include "ui/cbf_icon.hpp"
#include "autoplay/state.hpp"
#include "autoplay/persistence.hpp"

#include "autoplay/macro_format.hpp"

#include "autoplay/macro_loading.hpp"

#include "autoplay/macro_apply.hpp"

#include "autoplay/online_macros.hpp"

#include "autoplay/macro_choices.hpp"

#include "autoplay/brain_fetch.hpp"

#include "autoplay/load_saved.hpp"

#include "autoplay/generating.hpp"

#include "autoplay/cbf_compat.hpp"

#include "autoplay/control.hpp"

#include "autoplay/short_planner.hpp"

#include "autoplay/button.hpp"

#include "autoplay/brain_record.hpp"

#include "autoplay/inputs.hpp"

#include "autoplay/macro_best.hpp"

#include "autoplay/on_death.hpp"

#include "autoplay/on_finished.hpp"

#include "route/route_types.hpp"
#include "route/snapshots.hpp"

#include "route/route_core.hpp"

#include "route/route_sprint.hpp"

#include "route/beam_search.hpp"

#include "route/pathfinder.hpp"

#include "route/route_search.hpp"

#include "route/step_check.hpp"

#include "route/route_tick.hpp"

#include "route/object_index.hpp"

#include "autoplay/macro_timing.hpp"

#include "autoplay/macro_guard.hpp"

#include "autoplay/macro_play.hpp"

#include "autoplay/macro_step.hpp"

#include "autoplay/macro_fixes.hpp"

#include "autoplay/autoplay_tick.hpp"

#include "decision/auto_release.hpp"

#include "decision/auto_click.hpp"

#include "decision/deliver_pending.hpp"
}

#include "ui/macro_picker.hpp"

#include "ui/buffer_menu/buffer_menu.hpp"
#include "hooks/end_level_layer.hpp"

#include "ui/pause_button.hpp"

#include "core/on_load_settings.hpp"

#include "hooks/play_layer.hpp"

#include "hooks/editor_layer.hpp"

#include "hooks/hard_streak.hpp"

#include "hooks/effect_game_object.hpp"

#include "hooks/game_object.hpp"

#include "hooks/ring_object.hpp"

#include "hooks/player_object.hpp"

// v2.81: Geode keybinds (mod settings or Geode's keybinds popup; work without F keys too).
// Only act inside a level.
namespace {
#include "input/keybinds.hpp"

#include "net/update_check.hpp"
}

#include "core/on_load_services.hpp"

#include "hooks/menu_layer.hpp"

#include "hooks/base_game_layer.hpp"
