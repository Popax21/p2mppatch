#ifndef H_P2MPPATCH_PATCHES_ENV_FADE
#define H_P2MPPATCH_PATCHES_ENV_FADE

#include "plugin.hpp"
#include "byteseq.hpp"
#include "patch.hpp"
#include "anchors.hpp"

namespace patches {
    class CEnvFadePatch : public IPatchRegistrar {
        public:
            virtual const char *name() const { return "env_fade"; }

            virtual void register_patches(CMPPatchPlugin& plugin) {
                //Patch CEnvFade to never apply a fade to all clients (ignoring SF_FADE_ONLYONE)
                //We can get away with not checking whether we should intervene because these should only apply in the SP campaign
                SAnchor CEnvFade_InputFade = anchors::server::CEnvFade::InputFade.get(plugin.server_module());
                plugin.register_patch<CPatch>(CEnvFade_InputFade + 0x33, new SEQ_HEX("74 6b"), new SEQ_HEX("90 90"));

                SAnchor CEnvFade_InputReverseFade = anchors::server::CEnvFade::InputReverseFade.get(plugin.server_module());
                plugin.register_patch<CPatch>(CEnvFade_InputReverseFade + 0x109, new SEQ_HEX("0f 85 49 ff ff ff"), new SEQ_HEX("e9 85 49 ff ff ff"));
            }
    };
};

#endif