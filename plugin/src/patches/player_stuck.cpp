#include <tier0/dbg.h>
#include <const.h>
#include <tier0/valve_minmax_off.h>

#include <set>
#include "detour.hpp"
#include "anchors.hpp"
#include "utils.hpp"
#include "player_stuck.hpp"

using namespace patches;

int CPlayerStuckPatch::OFF_CBasePlayer_m_StuckLast;

void CPlayerStuckPatch::register_patches(CMPPatchPlugin& plugin) {
    OFF_CBasePlayer_m_StuckLast = get_server_CBasePlayer_m_StuckLast_offset(plugin.server_module());

    //Detour the start of CGameMovement::CheckStuck
    SAnchor CGameMovement_CheckStuck = PATCH_FUNC_ANCHOR(plugin.server_module(), CGameMovement::CheckStuck);
    plugin.register_patch<CPatch>(CGameMovement_CheckStuck + 0xa, new SEQ_MASKED_HEX("a1 ?? ?? ?? ??"),
        new SEQ_DETOUR_COPY_ORIG(plugin, 5, detour_CGameMovement_CheckStuck, DETOUR_ARG_STACK(0xb0), DETOUR_ARG_ESI, DETOUR_ARG_EIP)
    );

    //Detour the portal_use_player_avoidance cvar check in CPortal_Player::ShouldCollide
    uint8_t bool_val_off = (uint8_t) OFF_ConVar_boolValue;
    SAnchor CPortal_Player_ShouldCollide = PATCH_FUNC_ANCHOR(plugin.server_module(), CPortal_Player::ShouldCollide);
    plugin.register_patch<CPatch>(CPortal_Player_ShouldCollide + 0xb, new SEQ_HEX("8b 44 24 0c 8b 5c 24 10 8b 52 $1", &bool_val_off), new SEQ_SEQ(
        new SEQ_DETOUR_COPY_ORIG(plugin, 8, detour_CPortal_Player_ShouldCollide, DETOUR_ARG_ECX, DETOUR_ARG_EDX, DETOUR_ARG_EAX, DETOUR_ARG_EDX),
        new SEQ_FILL(3, 0x90)
    ));
}

static int ignore_inter_player_col = 0;

void CPlayerStuckPatch::detour_CGameMovement_CheckStuck(void ***ptr_movement, int *ptr_retval, void **ptr_eip) {
    void **movement = *ptr_movement;
    void *player = movement[1];
    int& m_StuckLast = *(int*) ((uint8_t*) player + OFF_CBasePlayer_m_StuckLast);
    // DevMsg("DETOUR CPlayerStuckPatch | CGameMovement::CheckStuck | this=%p player=%p m_StuckLast=%d\n", movement, player, m_StuckLast);

    //Decrement the ignore-player-collisions timer
    if(ignore_inter_player_col > 0) {
        ignore_inter_player_col--;
        DevMsg("CPlayerStuckPatch | Inter-player collision ignore timer ticked %d -> %d\n", ignore_inter_player_col+1, ignore_inter_player_col);
    }

    //Preserve the stuck status if the CPortal_Player::ShouldCollide detour tells us to (-> m_StuckLast is negative)
    //This code isn't run for noclip players, but eh
    if(m_StuckLast != -1) {
        if(m_StuckLast < 0) {
            Msg("P2MPPatch | Resetting inter-player collisions for CPortal_Player %p because player is no longer stuck\n", player);
            m_StuckLast = 0;
        }
        return;
    }

    m_StuckLast = -2; //Don't leave it at -1 as then we'll be stuck forever
    *ptr_retval = 0;
    *ptr_eip = (uint8_t*) *ptr_eip + 0x1a5; //Jump forward to a return block
}

void CPlayerStuckPatch::detour_CPortal_Player_ShouldCollide(void **ptr_player, void **ptr_playerAvoidanceCvar, int *ptr_collisionGroup, int *ptr_shouldIgnorePlayerCol) {
    void *player = *ptr_player;
    void *playerAvoidanceCvar = *ptr_playerAvoidanceCvar;
    int collisionGroup = *ptr_collisionGroup;
    int& m_StuckLast = *(int*) ((uint8_t*) player + OFF_CBasePlayer_m_StuckLast);
    DevMsg("DETOUR CPlayerStuckPatch | CPortal_Player::ShouldCollide | this=%p playerAvoidanceCvar=%p collisionGroup=%d m_StuckLast=%d ignore-col-timer=%d\n", player, playerAvoidanceCvar, collisionGroup, m_StuckLast, ignore_inter_player_col);

    //Get the value of the cvar
    if(*(uint32_t*) ((uint8_t*) playerAvoidanceCvar + OFF_ConVar_boolValue) != 0) {
        *ptr_shouldIgnorePlayerCol = 1;
        return;
    }

    //Check if we're colliding with another player
    if(collisionGroup == COLLISION_GROUP_PLAYER || collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT) {
        //Check if the player is stuck, or if we are ignoring player collisions in general
        if(m_StuckLast != 0 || ignore_inter_player_col > 0) {
            if(m_StuckLast >= 0) {
                Msg("P2MPPatch | Ignoring inter-player collisions for CPortal_Player %p because player is stuck\n", player);
                ignore_inter_player_col += 4; //Ignore inter-player collisions in general for a few frames
            }

            m_StuckLast = -1; //This will make the CheckStuck detour preserve our stuck status for the next check
            *ptr_shouldIgnorePlayerCol = 1;
            return;
        }
    }

    *ptr_shouldIgnorePlayerCol = 0;
}