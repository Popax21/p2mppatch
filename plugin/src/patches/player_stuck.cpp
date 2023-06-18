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

int CPlayerStuckPatch::OFF_ConVar_boolValue, CPlayerStuckPatch::OFF_CBaseEntity_m_MoveType, CPlayerStuckPatch::OFF_CBasePlayer_m_StuckLast;

CGlobalVars *CPlayerStuckPatch::gpGlobals;
void **CPlayerStuckPatch::ptr_g_pGameRules;
IServer *CPlayerStuckPatch::glob_sv;

void CPlayerStuckPatch::register_patches(CMPPatchPlugin& plugin) {
    OFF_CBaseEntity_m_MoveType = anchors::server::CBaseEntity::m_MoveType.get(plugin.server_module());
    OFF_CBasePlayer_m_StuckLast = anchors::server::CGameMovement::m_LastStuck.get(plugin.server_module());

    gpGlobals = plugin.get_globals();
    ptr_g_pGameRules = (void**) anchors::server::g_pGameRules.get(plugin.server_module()).get_addr();
    glob_sv = (IServer*) anchors::engine::sv.get(plugin.engine_module()).get_addr();

    //Detour the start of CGameMovement::CheckStuck
    SAnchor CGameMovement_CheckStuck = anchors::server::CGameMovement::CheckStuck.get(plugin.server_module());
    plugin.register_patch<CPatch>(CGameMovement_CheckStuck + 0xa, new SEQ_MASKED_HEX("a1 ?? ?? ?? ??"),
        new SEQ_DETOUR_COPY_ORIG(plugin, 5, detour_CGameMovement_CheckStuck, DETOUR_ARG_STACK(0xb0), DETOUR_ARG_ESI, DETOUR_ARG_EIP)
    );

    //Detour the portal_use_player_avoidance cvar check in CPortal_Player::ShouldCollide
    SAnchor CPortal_Player_ShouldCollide = anchors::server::CPortal_Player::ShouldCollide.get(plugin.server_module());
    plugin.register_patch<CPatch>(CPortal_Player_ShouldCollide + 0xb, new SEQ_MASKED_HEX("8b 44 24 0c 8b 5c 24 10 8b 52 ??"), new SEQ_SEQ(
        new SEQ_DETOUR_COPY_ORIG(plugin, 8, detour_CPortal_Player_ShouldCollide, DETOUR_ARG_ECX, DETOUR_ARG_EDX, DETOUR_ARG_EAX, DETOUR_ARG_EDX),
        new SEQ_FILL(3, 0x90)
    ));
    OFF_ConVar_boolValue = *((uint8_t*) CPortal_Player_ShouldCollide.get_addr() + 0xb + 10);
}

enum {
    STUCK_WAIT_PLAYER_COL_FIRST = -5,
    STUCK_WAIT_PLAYER_COL_LAST = -1,
    STUCK_HAD_PLAYER_COL_FIRST = -10,
    STUCK_HAD_PLAYER_COL_LAST = -6
};

void CPlayerStuckPatch::detour_CGameMovement_CheckStuck(void ***ptr_movement, int *ptr_retval, void **ptr_eip) {
    void **movement = *ptr_movement;
    void *player = movement[1];
    int& m_StuckLast = *(int*) ((uint8_t*) player + OFF_CBasePlayer_m_StuckLast);
    // DevMsg("DETOUR CPlayerStuckPatch | CGameMovement::CheckStuck | this=%p player=%p m_StuckLast=%d\n", movement, player, m_StuckLast);

    //Check if we should intervene
    if(glob_sv->GetNumClients() - glob_sv->GetNumProxies() <= (should_apply_sp_compat_patches(*ptr_g_pGameRules, gpGlobals) ? 1 : 2)) {
        return;
    }

    //Preserve the stuck status if the CPortal_Player::ShouldCollide detour tells us to
    if(m_StuckLast >= 0 || m_StuckLast == STUCK_WAIT_PLAYER_COL_FIRST) {
        if(m_StuckLast < 0) {
            Msg("P2MPPatch | Resetting inter-player collisions for CPortal_Player %p because player is no longer stuck\n", player);
            m_StuckLast = 0;
        }
        return;
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

    *ptr_retval = 0;
    *ptr_eip = (uint8_t*) *ptr_eip + 0x1a5; //Jump forward to a return block
}

void CPlayerStuckPatch::detour_CPortal_Player_ShouldCollide(void **ptr_player, void **ptr_playerAvoidanceCvar, int *ptr_collisionGroup, int *ptr_shouldIgnorePlayerCol) {
    void *player = *ptr_player;
    void *playerAvoidanceCvar = *ptr_playerAvoidanceCvar;
    int collisionGroup = *ptr_collisionGroup;
    int& m_StuckLast = *(int*) ((uint8_t*) player + OFF_CBasePlayer_m_StuckLast);
    // DevMsg("DETOUR CPlayerStuckPatch | CPortal_Player::ShouldCollide | this=%p playerAvoidanceCvar=%p collisionGroup=%d m_StuckLast=%d\n", player, playerAvoidanceCvar, collisionGroup, m_StuckLast);

    //Get the value of the cvar
    if(*(uint32_t*) ((uint8_t*) playerAvoidanceCvar + OFF_ConVar_boolValue) != 0) {
        *ptr_shouldIgnorePlayerCol = 1;
        return;
    }

    //Check if we should intervene
    if(glob_sv->GetNumClients() - glob_sv->GetNumProxies() <= (should_apply_sp_compat_patches(*ptr_g_pGameRules, gpGlobals) ? 1 : 2)) {
        *ptr_shouldIgnorePlayerCol = 0;
        return;
    }

    //Check if we're colliding with another player
    if(collisionGroup == COLLISION_GROUP_PLAYER || collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT) {
        //Check if the player is stuck
        if(m_StuckLast != 0) {
            if(m_StuckLast >= 0) {
                m_StuckLast = STUCK_HAD_PLAYER_COL_FIRST; //We go into a separate state for the first few frames upon being stuck in another player so that they are also picked up as being stuck in us
            } else if(STUCK_WAIT_PLAYER_COL_FIRST <= m_StuckLast && m_StuckLast <= STUCK_WAIT_PLAYER_COL_LAST) {
                m_StuckLast = STUCK_HAD_PLAYER_COL_FIRST + (m_StuckLast - STUCK_WAIT_PLAYER_COL_FIRST); //This will make the CheckStuck detour preserve our stuck status for the next check
            }

            *ptr_shouldIgnorePlayerCol = (m_StuckLast >= (STUCK_HAD_PLAYER_COL_FIRST + STUCK_HAD_PLAYER_COL_LAST) / 2) ? 1 : 0;
            return;
        }

        //Check if the player is in noclip
        if(*((uint8_t*) player + OFF_CBaseEntity_m_MoveType) == MOVETYPE_NOCLIP) {
            *ptr_shouldIgnorePlayerCol = 1;
            return;
        }
    }

    *ptr_shouldIgnorePlayerCol = 0;
}