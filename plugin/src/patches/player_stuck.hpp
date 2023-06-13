#ifndef H_P2MPPATCH_PATCHES_PLAYER_STUCK
#define H_P2MPPATCH_PATCHES_PLAYER_STUCK

#include <map>
#include "plugin.hpp"
#include "byteseq.hpp"
#include "patch.hpp"

namespace patches {
    class CPlayerStuckPatch : public IPatchRegistrar {
        public:
            virtual const char *name() const { return "player_stuck"; }

            virtual void register_patches(CMPPatchPlugin& plugin);

        private:
            static const int OFF_ConVar_boolValue = 0x30;
            static int OFF_CBasePlayer_m_StuckLast;

            static void detour_CGameMovement_CheckStuck(void ***ptr_movement, int *ptr_retval, void **ptr_eip);
            static void detour_CPortal_Player_ShouldCollide(void **ptr_player, void **ptr_playerAvoidanceCvar, int *ptr_collisionGroup, int *ptr_shouldIgnorePlayerCol);
    };
}

#endif