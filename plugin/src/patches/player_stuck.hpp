#ifndef H_P2MPPATCH_PATCHES_PLAYER_STUCK
#define H_P2MPPATCH_PATCHES_PLAYER_STUCK

#include <edict.h>
#include <tier0/valve_minmax_off.h>
#include "patch.hpp"
#include "hook.hpp"

class IServer;
class CMPPatchPlugin;

namespace patches {
    class CPlayerStuckPatch : public IPatchRegistrar {
        public:
            static const int NUM_NO_PLAYER_COL_FRAMES = 8;

            virtual const char *name() const override { return "player_stuck"; }

            virtual void register_patches(CMPPatchPlugin& plugin) override;

        private:
            static int OFF_CBasePlayer_m_StuckLast;

            static CGlobalVars *gpGlobals;
            static void **ptr_g_pGameRules;
            static IServer *glob_sv;

            static void __attribute__((regparm(1))) (*CBaseEntity_CollisionRulesChanged)(void *entity);

            HOOK_FUNC static int hook_CGameMovement_CheckStuck(HOOK_ORIG int (*orig)(void*), void **movement);
            HOOK_FUNC static int hook_CCollisionEvent_ShouldCollide(HOOK_ORIG bool (*orig)(void*, void*, void*, void*, void*), void *col_event, void *pObj0, void *pObj1, void *pGameData0, void *pGameData1);
            HOOK_FUNC static bool hook_CPortal_Player_ShouldCollide(HOOK_ORIG bool (*orig)(void*, int, int), void *player, int collisionGroup, int contentsMask);
    };
}

#endif