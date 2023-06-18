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

            virtual const char *name() const override { return "player_count"; }

            virtual void register_patches(CMPPatchPlugin& plugin) override {
                //Find CServerGameClients::GetPlayerLimits and patch maxplayers and defaultmaxplayers values
                uint8_t max_players_p1 = m_MaxPlayers + 1;
                SAnchor CServerGameClients_GetPlayerLimits = anchors::server::CServerGameClients::GetPlayerLimits.get(plugin.server_module());
                plugin.register_patch<CPatch>(CServerGameClients_GetPlayerLimits + 0x07, new SEQ_HEX("c7 00 01 00 00 00"), new SEQ_HEX("c7 00 $1 00 00 00", { &m_MaxPlayers }));
                plugin.register_patch<CPatch>(CServerGameClients_GetPlayerLimits + 0x11, new SEQ_HEX("c7 00 01 00 00 00"), new SEQ_HEX("c7 00 $1 00 00 00", { &m_MaxPlayers }));
                plugin.register_patch<CPatch>(CServerGameClients_GetPlayerLimits + 0x31, new SEQ_HEX("b8 02 00 00 00"), new SEQ_HEX("b8 $1 00 00 00", { &max_players_p1 }));

                //Find some player slot count ("members/numSlots") adjustion function
                SAnchor FUNC_slotcount_adj = anchors::server::FUNC_numSlots_adj.get(plugin.server_module());
                plugin.register_patch<CPatch>(FUNC_slotcount_adj + 0x701, new SEQ_HEX("be 01 00 00 00"), new SEQ_HEX("be $1 00 00 00", { &m_MaxPlayers })); //SP maps
                plugin.register_patch<CPatch>(FUNC_slotcount_adj + 0x6a2, new SEQ_HEX("be 02 00 00 00"), new SEQ_HEX("be $1 00 00 00", { &m_MaxPlayers })); //MP maps

                //Find CMatchTitleGameSettingsMgr::InitializeGameSettings and patch the "members/numSlots" value
                SAnchor CMatchTitleGameSettingsMgr_InitializeGameSettings = anchors::matchmaking::CMatchTitleGameSettingsMgr::InitializeGameSettings.get(plugin.matchmaking_module());
                plugin.register_patch<CPatch>(CMatchTitleGameSettingsMgr_InitializeGameSettings + 0x172, new SEQ_HEX("b8 01 00 00 00"), new SEQ_HEX("b8 $1 00 00 00", { &m_MaxPlayers }));
            }

            virtual void after_patch_status_change(CMPPatchPlugin& plugin, bool applied) override {
                //Invoke CGameServer::InitMaxClients to update the internal lower/upper maxplayers limits
                DevMsg("Invoking CGameServer::InitMaxClients...\n");
                SAnchor CGameServer_InitMaxClients = anchors::engine::CGameServer::InitMaxClients.get(plugin.engine_module());
                ((void *(*)(void*)) CGameServer_InitMaxClients.get_addr())(anchors::engine::sv.get(plugin.engine_module()).get_addr());
            }

        private:
            uint8_t m_MaxPlayers;
    };
};

#endif