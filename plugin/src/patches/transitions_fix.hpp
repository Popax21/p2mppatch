#ifndef H_P2MPPATCH_PATCHES_TRANSITIONS_FIX
#define H_P2MPPATCH_PATCHES_TRANSITIONS_FIX

#include "plugin.hpp"
#include "byteseq.hpp"
#include "patch.hpp"
#include "detour.hpp"

class IServer;

namespace patches {
    class CTransitionsFixPatch : public IPatchRegistrar {
        public:
            virtual const char *name() const { return "transitions_fix"; }

            virtual void register_patches(CMPPatchPlugin& plugin);

        private:
            static struct SPlayerSlots {
                static const int MAX_SLOT = 65534, INV_SLOT = 65535;

                struct {
                    uint16_t next_slot;
                    void *player;
                } slots[MAX_SLOT+1];
                uint16_t free_slots_head;

                SPlayerSlots() {
                    free_slots_head = MAX_SLOT;
                    for(int i = 1; i <= MAX_SLOT; i++) {
                        slots[i].next_slot = i-1;
                    }
                }

                void add_player_to_list(uint16_t& slot_list, void *player) {
                    if(free_slots_head <= 0) abort();

                    int player_slot = free_slots_head;
                    free_slots_head = slots[player_slot].next_slot;

                    slots[player_slot].next_slot = slot_list;
                    slots[player_slot].player = player;
                    slot_list = player_slot;
                }

                bool remove_player_from_list(uint16_t& slot_list, void *player) {
                    uint16_t *slot_ptr = &slot_list;
                    while(*slot_ptr > 0) {
                        uint16_t slot = *slot_ptr;
                        if(slots[slot].player == player) {
                            *slot_ptr = slots[slot].next_slot;
                            slots[slot].next_slot = free_slots_head;
                            free_slots_head = slot;
                            return true;
                        }
                        slot_ptr = &slots[slot].next_slot;
                    }
                    return false;
                }

                bool list_contains_player(uint16_t slot_list, void *player) const {
                    for(uint16_t slot = slot_list; slot > 0; slot = slots[slot].next_slot) {
                        if(slots[slot].player == player) return true;
                    }
                    return false;
                }

                int num_players_in_list(uint16_t slot_list) const {
                    int count = 0;
                    for(uint16_t slot = slot_list; slot > 0; slot = slots[slot].next_slot) count++;
                    return count;
                }

                void free_slot_list(uint16_t slot_list) {
                    for(uint16_t slot = slot_list; slot > 0;) {
                        uint16_t next_slot = slots[slot].next_slot;
                        slots[slot].next_slot = free_slots_head;
                        free_slots_head = slot;
                        slot = next_slot;
                    }
                }
            } player_slots;

            static const int OFF_CPortalMPGameRules_m_bDataReceived = 0x1c7d;

            static CGlobalVars *gpGlobals;
            static IServer *glob_sv;
            static void **ptr_g_pMatchFramework;

            static void *(*UTIL_PlayerByIndex)(int);
            static void (*CPortalMPGameRules_SendAllMapCompleteData)(void *rules);
            static void (*CPortalMPGameRules_StartPlayerTransitionThinks)(void *rules);
            static uint64_t (*CBaseEntity_ThinkSet)(void *ent, uint64_t func, float flNextThinkTime, const char *szContext); //The function pointers are 64 bit for some reason
            static void (*CBaseEntity_SetNextThink)(void *ent, float nextThinkTime, const char *szContext);
            static void (*CPortal_Player_PlayerTransitionCompleteThink)(void *player);

            static uint16_t& get_rules_slot_list(void *rules);
            static bool is_everyone_ready(void *rules, void *ignore_player = nullptr);

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