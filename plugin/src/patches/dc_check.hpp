#ifndef H_P2MPPATCH_PATCHES_DC_CHECK
#define H_P2MPPATCH_PATCHES_DC_CHECK

#include "plugin.hpp"
#include "byteseq.hpp"
#include "patch.hpp"
#include "anchors.hpp"

namespace patches {
    class CDCCheckPatch : public IPatchRegistrar {
        public:
            virtual const char *name() const { return "dc_check"; }

            virtual void register_patches(CMPPatchPlugin& plugin) {
                //Find CPortalMPGameRules::ClientDisconnected and remove disconnect check
                SAnchor CPortalMPGameRules_ClientDisconnected = PATCH_FUNC_ANCHOR(plugin.server_module(), CPortalMPGameRules::ClientDisconnected);
                plugin.register_patch<CPatch>(CPortalMPGameRules_ClientDisconnected + 0x8d, new SEQ_HEX("74"), new SEQ_HEX("eb"));

                //Find CPortal_Player::PlayerTransitionCompleteThink and remove disconnect check
                SAnchor CPortal_Player_PlayerTransitionCompleteThink = PATCH_FUNC_ANCHOR(plugin.server_module(), CPortal_Player::PlayerTransitionCompleteThink);
                plugin.register_patch<CPatch>(CPortal_Player_PlayerTransitionCompleteThink + 0x41, new SEQ_HEX("74 54"), new SEQ_HEX("eb 54"));
            }
    };
};

#endif