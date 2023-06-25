#ifndef H_P2MPPATCH_PATCHES_ENV_FADE
#define H_P2MPPATCH_PATCHES_ENV_FADE

#include "patch.hpp"
#include "detour.hpp"

class CGlobalVars;

namespace patches {
    class CEnvFadePatch : public IPatchRegistrar {
        public:
            virtual const char *name() const override { return "env_fade"; }

            virtual void register_patches(CMPPatchPlugin& plugin) override;

        private:
            static CGlobalVars *gpGlobals;
            static void **ptr_g_pGameRules;

            DETOUR_FUNC static void DETOUR_CEnvFade_InputFade_InputReverseFade(void ***ptr_inputdata, uint8_t *ptr_m_spawnflags);
    };
};

#endif