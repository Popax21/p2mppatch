#ifndef H_P2MPPATCH_PATCHES_PLAYER_COUNT
#define H_P2MPPATCH_PATCHES_PLAYER_COUNT

#include "plugin.hpp"
#include "patch.hpp"

namespace patches {
    class CPlayerCountPatch : public IPatchRegistrar {
        public:
            CPlayerCountPatch(uint8_t max_players) : m_MaxPlayers(max_players) {}

            virtual const char *name() const { return "player_count"; }

            virtual void register_patches(CMPPatchPlugin& plugin) {
                //Find CServerGameClients::GetPlayerLimits and patch maxplayers and defaultmaxplayers values
                uint8_t max_players_p1 = m_MaxPlayers + 1;
                SAnchor CServerGameClients_GetPlayerLimits = plugin.server_module().find_seq_anchor(CHexSequence("83 ec 0c 8b 44 24 1c c7 00 01 00 00 00 8b 44 24 14 c7 00 01 00 00 00"));
                DevMsg("&(CServerGameClients::GetPlayerLimits) = %p\n", CServerGameClients_GetPlayerLimits.get_addr());
                plugin.register_patch<CPatch>(CServerGameClients_GetPlayerLimits + 0x07, CHexSequence("c7 00 01 00 00 00"), CHexSequence("c7 00 $1 00 00 00", { &m_MaxPlayers }));
                plugin.register_patch<CPatch>(CServerGameClients_GetPlayerLimits + 0x31, CHexSequence("b8 02 00 00 00"), CHexSequence("b8 $1 00 00 00", { &max_players_p1 }));

                //Find some player slot count ("members/numSlots") adjustion function
                SAnchor FUNC_slotcount_adj = plugin.server_module().find_seq_anchor(CHexSequence("83 ec 0c 8b 10 50 ff 52 10 83 c4 10 89 c7 85 c0 74 12")) - 0x75;
                DevMsg("&FUNC_slotcount_adj = %p\n", FUNC_slotcount_adj.get_addr());
                plugin.register_patch<CPatch>(FUNC_slotcount_adj + 0x701, CHexSequence("be 01 00 00 00"), CHexSequence("be $1 00 00 00", { &m_MaxPlayers })); //SP maps
                plugin.register_patch<CPatch>(FUNC_slotcount_adj + 0x6a2, CHexSequence("be 02 00 00 00"), CHexSequence("be $1 00 00 00", { &m_MaxPlayers })); //MP maps

                //Find CMatchTitleGameSettingsMgr::InitializeGameSettings and patch the "members/numSlots" value
                SAnchor CMatchTitleGameSettingsMgr_InitializeGameSettings = plugin.matchmaking_module().find_seq_anchor(CHexSequence("57 56 53 8b 5c 24 14 8b 74 24 18 83 ec 04"));
                DevMsg("&(CMatchTitleGameSettingsMgr::InitializeGameSettings) = %p\n", CMatchTitleGameSettingsMgr_InitializeGameSettings.get_addr());
                plugin.register_patch<CPatch>(CMatchTitleGameSettingsMgr_InitializeGameSettings + 0x172, CHexSequence("b8 01 00 00 00"), CHexSequence("b8 $1 00 00 00", { &m_MaxPlayers }));

            }

        private:
            uint8_t m_MaxPlayers;
    };
};

#endif