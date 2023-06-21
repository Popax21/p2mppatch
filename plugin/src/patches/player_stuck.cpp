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

int CPlayerStuckPatch::OFF_CBaseEntity_m_MoveType, CPlayerStuckPatch::OFF_CBasePlayer_m_StuckLast;

CGlobalVars *CPlayerStuckPatch::gpGlobals;
void **CPlayerStuckPatch::ptr_g_pGameRules;
IServer *CPlayerStuckPatch::glob_sv;

void CPlayerStuckPatch::register_patches(CMPPatchPlugin& plugin) {
    //Obtain needed misc information for the hooks
    OFF_CBaseEntity_m_MoveType = anchors::server::CBaseEntity::m_MoveType.get(plugin.server_module());
    OFF_CBasePlayer_m_StuckLast = anchors::server::CGameMovement::m_LastStuck.get(plugin.server_module());

    gpGlobals = plugin.get_globals();
    ptr_g_pGameRules = (void**) anchors::server::g_pGameRules.get(plugin.server_module()).get_addr();
    glob_sv = (IServer*) anchors::engine::sv.get(plugin.engine_module()).get_addr();

    //Hook into CGameMovement::CheckStuck and CPortal_Player::ShouldCollide
    CHookTracker& htrack_CGameMovement_CheckStuck = anchors::server::CGameMovement::CheckStuck.hook_tracker(plugin.server_module());
    plugin.register_patch<CFuncHook>(plugin.scratchpad(), htrack_CGameMovement_CheckStuck, (void*) &hook_CGameMovement_CheckStuck, -100000);

    CHookTracker& htrack_CPortal_Player_ShouldCollide = anchors::server::CPortal_Player::ShouldCollide.hook_tracker(plugin.server_module());
    plugin.register_patch<CFuncHook>(plugin.scratchpad(), htrack_CPortal_Player_ShouldCollide, (void*) &hook_CPortal_Player_ShouldCollide);
}

enum {
    STUCK_WAIT_PLAYER_COL_FIRST = -5,
    STUCK_WAIT_PLAYER_COL_LAST = -1,
    STUCK_HAD_PLAYER_COL_FIRST = -10,
    STUCK_HAD_PLAYER_COL_LAST = -6
};

HOOK_FUNC int CPlayerStuckPatch::hook_CGameMovement_CheckStuck(HOOK_ORIG int (*orig)(void*), void **movement) {
    void *player = movement[1];
    int& m_StuckLast = *(int*) ((uint8_t*) player + OFF_CBasePlayer_m_StuckLast);

    //Check if we should intervene
    if(glob_sv->GetNumClients() - glob_sv->GetNumProxies() <= (should_apply_sp_compat_patches(*ptr_g_pGameRules, gpGlobals) ? 1 : 2)) {
        return orig(movement);
    }

    //Preserve the stuck status if the CPortal_Player::ShouldCollide detour tells us to
    if(m_StuckLast >= 0 || m_StuckLast == STUCK_WAIT_PLAYER_COL_FIRST) {
        if(m_StuckLast < 0) {
            Msg("P2MPPatch | Resetting inter-player collisions for CPortal_Player %p because player is no longer stuck\n", player);
            m_StuckLast = 0;
        }
        return orig(movement);
    }

    if(STUCK_WAIT_PLAYER_COL_FIRST < m_StuckLast && m_StuckLast <= STUCK_WAIT_PLAYER_COL_LAST) {
        //We didn't collide with a player during this check
        //Move back to the previous wait phase
        m_StuckLast--;
        DevMsg("CPlayerStuckPatch | Decremented CPortal_Player %p stuck state to %d\n", player, m_StuckLast);
    } else if(STUCK_HAD_PLAYER_COL_FIRST <= m_StuckLast && m_StuckLast <= STUCK_HAD_PLAYER_COL_LAST) {
        //We collided with a player during this check
        //Advance to the next wait phase
        if(m_StuckLast < STUCK_HAD_PLAYER_COL_LAST) {
            if(m_StuckLast == STUCK_HAD_PLAYER_COL_FIRST) Msg("P2MPPatch | Ignoring inter-player collisions for CPortal_Player %p because player is stuck\n", player);
            m_StuckLast = STUCK_WAIT_PLAYER_COL_FIRST + (m_StuckLast - STUCK_HAD_PLAYER_COL_FIRST) + 1;
            DevMsg("CPlayerStuckPatch | Incremented CPortal_Player %p stuck state to %d\n", player, m_StuckLast);
        } else m_StuckLast = STUCK_WAIT_PLAYER_COL_LAST;
    }

    return 0;
}

HOOK_FUNC bool CPlayerStuckPatch::hook_CPortal_Player_ShouldCollide(HOOK_ORIG bool (*orig)(void*, int, int), void *player, int collisionGroup, int contentsMask) {
    int& m_StuckLast = *(int*) ((uint8_t*) player + OFF_CBasePlayer_m_StuckLast);

    //Check if we're colliding with another player
    if(collisionGroup != COLLISION_GROUP_PLAYER && collisionGroup != COLLISION_GROUP_PLAYER_MOVEMENT) {    
        return orig(player, collisionGroup, contentsMask);
    }

    //Check if we should intervene
    if(glob_sv->GetNumClients() - glob_sv->GetNumProxies() <= (should_apply_sp_compat_patches(*ptr_g_pGameRules, gpGlobals) ? 1 : 2)) {
        return orig(player, collisionGroup, contentsMask);
    }

    //Check if the player is in noclip
    if(*((uint8_t*) player + OFF_CBaseEntity_m_MoveType) == MOVETYPE_NOCLIP) {
        return false;
    }

    //Check if the player is stuck
    if(m_StuckLast != 0) {
        if(m_StuckLast >= 0) {
            m_StuckLast = STUCK_HAD_PLAYER_COL_FIRST; //We go into a separate state for the first few frames upon being stuck in another player so that they are also picked up as being stuck in us
        } else if(STUCK_WAIT_PLAYER_COL_FIRST <= m_StuckLast && m_StuckLast <= STUCK_WAIT_PLAYER_COL_LAST) {
            m_StuckLast = STUCK_HAD_PLAYER_COL_FIRST + (m_StuckLast - STUCK_WAIT_PLAYER_COL_FIRST); //This will make the CheckStuck detour preserve our stuck status for the next check
        }

        if(m_StuckLast >= (STUCK_HAD_PLAYER_COL_FIRST + STUCK_HAD_PLAYER_COL_LAST) / 2) return false;
    }

    return orig(player, collisionGroup, contentsMask);
}