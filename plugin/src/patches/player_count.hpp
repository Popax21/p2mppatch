#ifndef H_P2MPPATCH_PATCHES_PLAYER_COUNT
#define H_P2MPPATCH_PATCHES_PLAYER_COUNT

#include "plugin.hpp"
#include "byteseq.hpp"
#include "patch.hpp"
#include "anchors.hpp"

namespace patches {
    class CPlayerCountPatch : public IPatchRegistrar {
        public:
            CPlayerCountPatch(uint8_t max_players) : m_MaxPlayers(max_players) {}

            virtual const char *name() const { return "player_count"; }

            virtual void register_patches(CMPPatchPlugin& plugin) {
                //Find CServerGameClients::GetPlayerLimits and patch maxplayers and defaultmaxplayers values
                uint8_t max_players_p1 = m_MaxPlayers + 1;
                SAnchor CServerGameClients_GetPlayerLimits = PATCH_FUNC_ANCHOR(plugin.server_module(), CServerGameClients::GetPlayerLimits);
                plugin.register_patch<CPatch>(CServerGameClients_GetPlayerLimits + 0x07, new SEQ_HEX("c7 00 01 00 00 00"), new SEQ_HEX("c7 00 $1 00 00 00", { &m_MaxPlayers }));
                plugin.register_patch<CPatch>(CServerGameClients_GetPlayerLimits + 0x31, new SEQ_HEX("b8 02 00 00 00"), new SEQ_HEX("b8 $1 00 00 00", { &max_players_p1 }));

                //Find some player slot count ("members/numSlots") adjustion function
                SAnchor FUNC_slotcount_adj = PATCH_FUNC_ANCHOR(plugin.server_module(), FUNC_numSlots_adj);
                plugin.register_patch<CPatch>(FUNC_slotcount_adj + 0x701, new SEQ_HEX("be 01 00 00 00"), new SEQ_HEX("be $1 00 00 00", { &m_MaxPlayers })); //SP maps
                plugin.register_patch<CPatch>(FUNC_slotcount_adj + 0x6a2, new SEQ_HEX("be 02 00 00 00"), new SEQ_HEX("be $1 00 00 00", { &m_MaxPlayers })); //MP maps

                //Find CMatchTitleGameSettingsMgr::InitializeGameSettings and patch the "members/numSlots" value
                SAnchor CMatchTitleGameSettingsMgr_InitializeGameSettings = PATCH_FUNC_ANCHOR(plugin.matchmaking_module(), CMatchTitleGameSettingsMgr::InitializeGameSettings);
                plugin.register_patch<CPatch>(CMatchTitleGameSettingsMgr_InitializeGameSettings + 0x172, new SEQ_HEX("b8 01 00 00 00"), new SEQ_HEX("b8 $1 00 00 00", { &m_MaxPlayers }));
            }

        private:
            uint8_t m_MaxPlayers;
    };
};

#endif