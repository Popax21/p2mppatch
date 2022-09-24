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
                SAnchor CServerGameClients_GetPlayerLimits = plugin.server_module().find_seq_anchor(CHexSequence("83 ec 08 8b 44 24 1c c7 00 01 00 00 00 8b 44 24 14 c7 00 01 00 00 00")) - 0x0c;
                DevMsg("&(CServerGameClients::GetPlayerLimits) = %p\n", CServerGameClients_GetPlayerLimits.get_addr());
                plugin.register_patch<CPatch>(CServerGameClients_GetPlayerLimits + 0x13, CHexSequence("c7 00 01 00 00 00"), CHexSequence("c7 00 $1 00 00 00", { &m_MaxPlayers }));
                plugin.register_patch<CPatch>(CServerGameClients_GetPlayerLimits + 0x44, CHexSequence("83 c0 02"), CHexSequence("83 c0 $1", { &m_MaxPlayers }));

                //Find some player slot count adjustion function
                SAnchor FUNC_slotcount_adj = plugin.server_module().find_seq_anchor(CHexSequence("81 ec 2c 01 00 00 8b 7d 0c 65 a1 14 00 00 00 89 45 e4 31 c0 85 ff")) - 0x11;
                DevMsg("&FUNC_slotcount_adj = %p\n", FUNC_slotcount_adj.get_addr());
                plugin.register_patch<CPatch>(FUNC_slotcount_adj + 0x917, CHexSequence("b8 01 00 00 00"), CHexSequence("b8 $1 00 00 00", { &m_MaxPlayers })); //SP maps
                plugin.register_patch<CPatch>(FUNC_slotcount_adj + 0x8b3, CHexSequence("83 c0 02"), CHexSequence("83 c0 $1", { &m_MaxPlayers })); //MP maps

                //Find CMatchTitleGameSettingsMgr::InitializeGameSettings and patch "members/numSlots" value
                SAnchor CMatchTitleGameSettingsMgr_InitializeGameSettings = plugin.matchmaking_module().find_seq_anchor(CHexSequence("83 c4 0c 85 c0 0f 95 c0 0f b6 c0 83 c0 01 50")) - 0x196;
                DevMsg("&(CMatchTitleGameSettingsMgr::InitializeGameSettings) = %p\n", CMatchTitleGameSettingsMgr_InitializeGameSettings.get_addr());
                plugin.register_patch<CPatch>(CMatchTitleGameSettingsMgr_InitializeGameSettings + 0x1a1, CHexSequence("83 c0 01"), CHexSequence("83 c0 $1", { &m_MaxPlayers }));

            }

        private:
            uint8_t m_MaxPlayers;
    };
};

#endif