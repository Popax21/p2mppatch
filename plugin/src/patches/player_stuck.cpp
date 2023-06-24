#include <tier0/dbg.h>
#include <iserver.h>
#include <const.h>
#include <tier0/valve_minmax_off.h>

#include "detour.hpp"
#include "plugin.hpp"
#include "anchors.hpp"
#include "player_stuck.hpp"
#include "utils.hpp"

using namespace patches;

int CPlayerStuckPatch::OFF_CBasePlayer_m_StuckLast;

CGlobalVars *CPlayerStuckPatch::gpGlobals;
void **CPlayerStuckPatch::ptr_g_pGameRules;
IServer *CPlayerStuckPatch::glob_sv;

void __attribute__((regparm(1))) (*CPlayerStuckPatch::CBaseEntity_CollisionRulesChanged)(void *entity);

void CPlayerStuckPatch::register_patches(CMPPatchPlugin& plugin) {
    //Obtain needed misc information for the hooks
    OFF_CBasePlayer_m_StuckLast = anchors::server::CGameMovement::m_LastStuck.get(plugin.server_module());

    gpGlobals = plugin.get_globals();
    ptr_g_pGameRules = (void**) anchors::server::g_pGameRules.get(plugin.server_module()).get_addr();
    glob_sv = (IServer*) anchors::engine::sv.get(plugin.engine_module()).get_addr();

    CBaseEntity_CollisionRulesChanged = (void __attribute__((regparm(1)))  (*)(void*)) anchors::server::CBaseEntity::CollisionRulesChanged.get(plugin.server_module()).get_addr();

    //Hook into CGameMovement::CheckStuck (to check if we're stuck in another player), CCollisionEvent::ShouldCollide and CPortal_Player::ShouldCollide (to disable collisions if that's the case)
    CHookTracker& htrack_CGameMovement_CheckStuck = anchors::server::CGameMovement::CheckStuck.hook_tracker(plugin.server_module());
    plugin.register_patch<CFuncHook>(plugin.scratchpad(), htrack_CGameMovement_CheckStuck, (void*) &hook_CGameMovement_CheckStuck, -100000);

    CHookTracker& htrack_CCollisionEvent_ShouldCollide = anchors::server::CCollisionEvent::ShouldCollide.hook_tracker(plugin.server_module());
    plugin.register_patch<CFuncHook>(plugin.scratchpad(), htrack_CCollisionEvent_ShouldCollide, (void*) &hook_CCollisionEvent_ShouldCollide);

    CHookTracker& htrack_CPortal_Player_ShouldCollide = anchors::server::CPortal_Player::ShouldCollide.hook_tracker(plugin.server_module());
    plugin.register_patch<CFuncHook>(plugin.scratchpad(), htrack_CPortal_Player_ShouldCollide, (void*) &hook_CPortal_Player_ShouldCollide);
}

static bool in_stuck_probe, stuck_probe_hit_player;

HOOK_FUNC int CPlayerStuckPatch::hook_CGameMovement_CheckStuck(HOOK_ORIG int (*orig)(void*), void **movement) {
    void *player = movement[1];
    int& m_StuckLast = *(int*) ((uint8_t*) player + OFF_CBasePlayer_m_StuckLast);

    //Check if we should intervene
    if(glob_sv->GetNumClients() - glob_sv->GetNumProxies() <= (should_apply_sp_compat_patches(*ptr_g_pGameRules, gpGlobals) ? 1 : 2)) {
        if(m_StuckLast < 0) m_StuckLast = 0;
        return orig(movement);
    }

    if(m_StuckLast < 0) {        
        //Decrement the no-player-collision "timer"
        m_StuckLast++;
        if(m_StuckLast < 0) return 0;
        else {
            Msg("P2MPPatch | Resetting inter-player collisions for CPortal_Player %p because player is no longer stuck\n", player);
            CBaseEntity_CollisionRulesChanged(player);
        }
    }

    //Do a stuck check
    int old_m_StuckLast = m_StuckLast;
    int is_stuck = orig(movement);
    if(!is_stuck) return 0;

    //Do a second stuck check to trigger additional collision checks, however track any player collisions
    m_StuckLast = old_m_StuckLast; //Ensure that we execute the exact same code path

    in_stuck_probe = true;
    stuck_probe_hit_player = false;
    is_stuck = orig(movement);
    in_stuck_probe = false;

    if(!is_stuck && !stuck_probe_hit_player) return 0; //Nevermind then

    if(stuck_probe_hit_player) {
        //Disable inter-player collisions for some time
        Msg("P2MPPatch | Ignoring inter-player collisions for CPortal_Player %p because player is stuck in another player\n", player);
        m_StuckLast = -NUM_NO_PLAYER_COL_FRAMES;
        CBaseEntity_CollisionRulesChanged(player);
        return 0; //Make the game think we're not actually stuck
    } else {
        //We're not stuck within another player
        return 1;
    }
}

HOOK_FUNC int CPlayerStuckPatch::hook_CCollisionEvent_ShouldCollide(HOOK_ORIG bool (*orig)(void*, void*, void*, void*, void*), void *col_event, void *pObj0, void *pObj1, void *pGameData0, void *pGameData1) {
    //Check if both entities are players
    if(!pGameData0 || !PATCH_VTAB_FUNC(pGameData0, server::CBaseEntity::IsPlayer)(pGameData0)) return orig(col_event, pObj0, pObj1, pGameData0, pGameData1);
    if(!pGameData1 || !PATCH_VTAB_FUNC(pGameData1, server::CBaseEntity::IsPlayer)(pGameData1)) return orig(col_event, pObj0, pObj1, pGameData0, pGameData1);

    //Check if both players have inter-player collisions disabled
    if(
        *(int*) ((uint8_t*) pGameData0 + OFF_CBasePlayer_m_StuckLast) < 0 &&
        *(int*) ((uint8_t*) pGameData1 + OFF_CBasePlayer_m_StuckLast) < 0
    ) return 0;

    return orig(col_event, pObj0, pObj1, pGameData0, pGameData1);
}

HOOK_FUNC bool CPlayerStuckPatch::hook_CPortal_Player_ShouldCollide(HOOK_ORIG bool (*orig)(void*, int, int), void *player, int collisionGroup, int contentsMask) {
    int& m_StuckLast = *(int*) ((uint8_t*) player + OFF_CBasePlayer_m_StuckLast);

    // //Check if we're checking collision with another player
    // //This might also include some other stuff, but eh
    // if(collisionGroup != COLLISION_GROUP_PLAYER && collisionGroup != COLLISION_GROUP_PLAYER_MOVEMENT && !(contentsMask & CONTENTS_PLAYERCLIP)) {
    //     return orig(player, collisionGroup, contentsMask);
    // }

    //Check if we should intervene
    if(glob_sv->GetNumClients() - glob_sv->GetNumProxies() <= (should_apply_sp_compat_patches(*ptr_g_pGameRules, gpGlobals) ? 1 : 2)) {
        return orig(player, collisionGroup, contentsMask);
    }

    //Check if we're in a collision probe
    if(in_stuck_probe) {
        stuck_probe_hit_player = true;

        Msg("P2MPPatch | Ignoring inter-player collisions for CPortal_Player %p because a player is stuck in them\n", player);
        m_StuckLast = -NUM_NO_PLAYER_COL_FRAMES; //Start the timer for this player as well
        CBaseEntity_CollisionRulesChanged(player);
        return false;
    }

    //Check if the no-player-collision timer is running
    if(m_StuckLast < 0) {
        m_StuckLast = -NUM_NO_PLAYER_COL_FRAMES;
        return false;
    }

    return orig(player, collisionGroup, contentsMask);
}