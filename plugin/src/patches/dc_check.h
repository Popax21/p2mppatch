#ifndef H_P2MPPATCH_PATCHES_DC_CHECK
#define H_P2MPPATCH_PATCHES_DC_CHECK

#include "plugin.hpp"
#include "patch.hpp"

namespace patches {
    class CDCCheckPatch : public IPatchRegistrar {
        public:
            virtual const char *name() const { return "dc_check"; }

            virtual void register_patches(CMPPatchPlugin& plugin) {
                //Find CPortalMPGameRules::ClientDisconnected and remove disconnect check
                SAnchor CPortalMPGameRules_ClientDisconnected = plugin.server_module().find_seq_anchor(SEQ_HEX("55 57 56 53 83 ec 28 8b 44 24 3c 8b 7c 24 40 89 44 24 18 57"));
                DevMsg("&(CPortalMPGameRules::ClientDisconnected) = %p\n", CPortalMPGameRules_ClientDisconnected.get_addr());
                plugin.register_patch<CPatch>(CPortalMPGameRules_ClientDisconnected + 0x82, new SEQ_HEX("8b 4c 24 0c"), new SEQ_HEX("66 e9 9d 00")); //Jump over the check

                //Find CPortal_Player::PlayerTransitionCompleteThink and remove disconnect check
                SAnchor CPortal_Player_PlayerTransitionCompleteThink = plugin.server_module().find_seq_anchor(SEQ_HEX("8b 5c 24 28 8b 10 68 00 00 f8 3f 6a 00")) - 0xa;
                DevMsg("&(CPortal_Player::PlayerTransitionCompleteThink) = %p\n", CPortal_Player_PlayerTransitionCompleteThink.get_addr());
                plugin.register_patch<CPatch>(CPortal_Player_PlayerTransitionCompleteThink + 0x41, new SEQ_HEX("74 54"), new SEQ_HEX("eb 54")); //Patch the if check to always fail
            }
    };
};

#endif