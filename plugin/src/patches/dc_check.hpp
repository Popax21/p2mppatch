#ifndef H_P2MPPATCH_PATCHES_DC_CHECK
#define H_P2MPPATCH_PATCHES_DC_CHECK

#include "plugin.hpp"
#include "byteseq.hpp"
#include "patch.hpp"
#include "anchors.hpp"

namespace patches {
    class CDCCheckPatch : public IPatchRegistrar {
        public:
            virtual const char *name() const override { return "dc_check"; }

            virtual void register_patches(CMPPatchPlugin& plugin) override {
                //Find CPortalMPGameRules::ClientDisconnected and remove the disconnect check
                SAnchor CPortalMPGameRules_ClientDisconnected = anchors::server::CPortalMPGameRules::ClientDisconnected.get(plugin.server_module());
                plugin.register_patch<CSeqPatch>(CPortalMPGameRules_ClientDisconnected + 0x8d, new SEQ_HEX("74"), new SEQ_HEX("eb"));

                //Find CPortal_Player::PlayerTransitionCompleteThink and remove the disconnect check
                SAnchor CPortal_Player_PlayerTransitionCompleteThink = anchors::server::CPortal_Player::PlayerTransitionCompleteThink.get(plugin.server_module());
                plugin.register_patch<CSeqPatch>(CPortal_Player_PlayerTransitionCompleteThink + 0x41, new SEQ_HEX("74 54"), new SEQ_HEX("eb 54"));
            }
    };
};

#endif