#ifndef H_P2MPPATCH_PATCHES_TRANSITIONS_FIX
#define H_P2MPPATCH_PATCHES_TRANSITIONS_FIX

#include <set>
#include "patch.hpp"
#include "detour.hpp"

class IServer;
class CMPPatchPlugin;

namespace patches {
    class CTransitionsFixPatch : public IPatchRegistrar {
        public:
            virtual const char *name() const { return "transitions_fix"; }

            virtual void register_patches(CMPPatchPlugin& plugin);

        private:
            static int OFF_CPortalMPGameRules_m_bDataReceived;

            static IServer *glob_sv;
            static void **ptr_g_pMatchFramework;

            static void *(*UTIL_PlayerByIndex)(int idx);
            static int (*KeyValues_GetInt)(void *kv, const char *keyName, int defaultValue);
            static void (*CPortalMPGameRules_SendAllMapCompleteData)(void *rules);
            static void (*CPortalMPGameRules_StartPlayerTransitionThinks)(void *rules);

            struct SReadyTracker {
                bool is_ready;
                int req_player_cnt;
                std::set<void*> ready_players;

                SReadyTracker() : is_ready(false), req_player_cnt(-1) {}

                void init_match_req_player_count();

                int get_req_players() const;
                bool is_everyone_ready() const;
            };

            static const int NUM_TRACKER_SLOTS = 65536;
            static SReadyTracker *tracker_slots[NUM_TRACKER_SLOTS];

            static uint16_t& get_rules_ready_tracker_slot(void *rules) { return *(uint16_t*) ((uint8_t*) rules + OFF_CPortalMPGameRules_m_bDataReceived); }
            static SReadyTracker *get_rules_ready_tracker(void *rules) { return tracker_slots[get_rules_ready_tracker_slot(rules)]; }

            DETOUR_FUNC static void detour_CPortalMPGameRules_CPortalMPGameRules(void **ptr_rules);
            DETOUR_FUNC static void detour_CPortalMPGameRules_destr_CPortalMPGameRules(void **ptr_rules);
            DETOUR_FUNC static void detour_CPortalMPGameRules_ClientCommandKeyValues_A(void **ptr_rules, void **ptr_pPlayer);
            DETOUR_FUNC static void detour_CPortalMPGameRules_ClientCommandKeyValues_B(void **ptr_rules, int *ptr_everyoneReady);
            DETOUR_FUNC static void detour_CPortalMPGameRules_ClientDisconnected(void **ptr_rules, void **ptr_pPlayer);
            DETOUR_FUNC static void detour_CPortalMPGameRules_SetMapCompleteData(void **ptr_rules, void **ptr_pPlayer, int *ptr_shouldContinue);
            DETOUR_FUNC static void detour_CPortal_Player_Spawn(void **ptr_rules, int *ptr_everyoneReady);
            DETOUR_FUNC static void detour_CPortal_Player_OnFullyConnected(void **ptr_rules, int *ptr_everyoneReady);
    };
};

#endif