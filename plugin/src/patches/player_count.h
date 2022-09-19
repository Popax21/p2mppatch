#ifndef H_P2MPPATCH_PATCHES_PLAYER_COUNT
#define H_P2MPPATCH_PATCHES_PLAYER_COUNT

#include "patch.hpp"

namespace patches {
    class CPlayerCountPatch : public IPatchRegistrar {
        public:
            CPlayerCountPatch(int max_players) : m_MaxPlayers(max_players) {}

            virtual const char *name() const { return "player_count"; }

            virtual void register_patches(const CMPPatchPlugin& plugin) {
                //Find CServerGameClients::GetPlayerLimits
                SAnchor CServerGameClients_GetPlayerLimits = plugin.server_module().find_seq_anchor(CHexSequence("83 ec 08 8b 44 24 1c c7 00 01 00 00 00 8b 44 24 14 c7 00 01 00 00 00")) - 0x0c;
                DevMsg("&(CServerGameClients::GetPlayerLimits) = %p\n", CServerGameClients_GetPlayerLimits.get_addr());
            
            }

        private:
            int m_MaxPlayers;
    };
};

#endif