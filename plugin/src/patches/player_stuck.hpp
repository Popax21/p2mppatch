#ifndef H_P2MPPATCH_PATCHES_PLAYER_STUCK
#define H_P2MPPATCH_PATCHES_PLAYER_STUCK

#include <map>
#include "patch.hpp"
#include "hook.hpp"

class IServer;
class CMPPatchPlugin;

namespace patches {
    class CPlayerStuckPatch : public IPatchRegistrar {
        public:
            virtual const char *name() const override { return "player_stuck"; }

            virtual void register_patches(CMPPatchPlugin& plugin) override;

        private:
            static int OFF_CBaseEntity_m_MoveType, OFF_CBasePlayer_m_StuckLast;

            static CGlobalVars *gpGlobals;
            static void **ptr_g_pGameRules;
            static IServer *glob_sv;

            HOOK_FUNC static int hook_CGameMovement_CheckStuck(HOOK_ORIG int (*orig)(void*), void **movement);
            HOOK_FUNC static bool hook_CPortal_Player_ShouldCollide(HOOK_ORIG bool (*orig)(void*, int, int), void *player, int collisionGroup, int contentsMask);
    };
}

#endif