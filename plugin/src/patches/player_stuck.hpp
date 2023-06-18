#ifndef H_P2MPPATCH_PATCHES_PLAYER_STUCK
#define H_P2MPPATCH_PATCHES_PLAYER_STUCK

#include <map>
#include "patch.hpp"

class IServer;
class CMPPatchPlugin;

namespace patches {
    class CPlayerStuckPatch : public IPatchRegistrar {
        public:
            virtual const char *name() const { return "player_stuck"; }

            virtual void register_patches(CMPPatchPlugin& plugin);

        private:
            static int OFF_ConVar_boolValue, OFF_CBaseEntity_m_MoveType, OFF_CBasePlayer_m_StuckLast;

            static CGlobalVars *gpGlobals;
            static void **ptr_g_pGameRules;
            static IServer *glob_sv;

            static void detour_CGameMovement_CheckStuck(void ***ptr_movement, int *ptr_retval, void **ptr_eip);
            static void detour_CPortal_Player_ShouldCollide(void **ptr_player, void **ptr_playerAvoidanceCvar, int *ptr_collisionGroup, int *ptr_shouldIgnorePlayerCol);
    };
}

#endif