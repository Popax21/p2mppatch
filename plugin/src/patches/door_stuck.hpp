#ifndef H_P2MPPATCH_PATCHES_DOOR_STUCK
#define H_P2MPPATCH_PATCHES_DOOR_STUCK

#include <edict.h>
#include <tier0/valve_minmax_off.h>
#include "patch.hpp"
#include "hook.hpp"

class IServer;
class CMPPatchPlugin;

namespace patches {
    class CDoorStuckPatch : public IPatchRegistrar {
        public:
            virtual const char *name() const override { return "door_stuck"; }

            virtual void register_patches(CMPPatchPlugin& plugin) override;

        private:
            static int OFF_CTriggerMultiple_m_flWait, OFF_CTriggerMultiple_m_OnTrigger;
            static int OFF_CBaseEntityOutput_m_ActionList;

            static CGlobalVars *gpGlobals;
            static void **ptr_g_pGameRules;

            HOOK_FUNC static void hook_CTriggerOnce_Spawn(HOOK_ORIG int (*orig)(void*), void *trigger);
    };
}

#endif