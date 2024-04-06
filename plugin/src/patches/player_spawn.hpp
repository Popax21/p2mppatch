#ifndef H_P2MPPATCH_PATCHES_PLAYER_SPAWN
#define H_P2MPPATCH_PATCHES_PLAYER_SPAWN

#include "patch.hpp"
#include "detour.hpp"

class Vector;
class QAngle;
class IServer;
class CGlobalVars;
class CMPPatchPlugin;

namespace patches {
    class CPlayerSpawnPatch : public IPatchRegistrar {
        public:
            virtual const char *name() const override { return "player_spawn"; }

            virtual void register_patches(CMPPatchPlugin& plugin) override;

        private:
            static int OFF_CBaseEntity_m_MoveType, OFF_CBaseEntity_m_CollisionGroup;

            static void *engine_trace;
            static CGlobalVars *gpGlobals;
            static void **ptr_g_pGameRules;
            static void **ptr_g_pLastSpawn;

            static void *(*UTIL_PlayerByIndex)(int idx);
            static void *(*CBasePlayer_EntSelectSpawnPoint)(void *player);
            static void (*CPointTeleport_DoTeleport)(void *teleport, void *inputdata, Vector *vecOrigin, QAngle *angRotation, uintptr_t bOverrideTarget);

            DETOUR_FUNC static void detour_CPointTeleport_DoTeleport(void **ptr_teleport, const char **ptr_entName, Vector **ptr_vecOrigin, QAngle **ptr_angRotation);
    };
}

#endif