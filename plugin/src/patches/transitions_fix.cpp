#include <tier0/dbg.h>
#include "byteseq.hpp"
#include "transitions_fix.hpp"
#include "anchors.hpp"

using namespace patches;

CTransitionsFixPatch::SPlayerSlots CTransitionsFixPatch::player_slots;

void *(*CTransitionsFixPatch::UTIL_PlayerByIndex)(int);
int (*CTransitionsFixPatch::GetNumPlayersConnected)();
void (*CTransitionsFixPatch::CPortalMPGameRules_SendAllMapCompleteData)(void *rules);
void (*CTransitionsFixPatch::CPortalMPGameRules_StartPlayerTransitionThinks)(void *rules);

void CTransitionsFixPatch::register_patches(CMPPatchPlugin& plugin) {
    //Find CPortalMPGameRules::CPortalMPGameRules and set m_bMapNamesLoaded to true by default
    //This causes the level_complete_data -> read_stats -> StartPlayerTransitionThinks cascade to happen for SP maps as well
    SAnchor CPortalMPGameRules_CPortalMPGameRules = PATCH_FUNC_ANCHOR(plugin.server_module(), CPortalMPGameRules::CPortalMPGameRules);
    plugin.register_patch<CPatch>(CPortalMPGameRules_CPortalMPGameRules + 0x3aa, new SEQ_HEX("80 bf bc 1b 00 00 00 0f 85 59 01 00 00"), new SEQ_HEX("c6 87 7c 02 00 00 01 90 90 90 90 90 90"));

    //Rewire the functionality of m_bDataReceived
    //This bool[2] array is used to track whether all players are ready during a transition
    //Replace it with an arbitrary-size linked list, reusing the 2 bytes as the uint16_t head index
    SAnchor CPortalMPGameRules_destr_CPortalMPGameRules = PATCH_FUNC_ANCHOR(plugin.server_module(), CPortalMPGameRules::destr_CPortalMPGameRules);
    SAnchor CPortalMPGameRules_ClientCommandKeyValues = PATCH_FUNC_ANCHOR(plugin.server_module(), CPortalMPGameRules::ClientCommandKeyValues);
    SAnchor CPortalMPGameRules_ClientDisconnected = PATCH_FUNC_ANCHOR(plugin.server_module(), CPortalMPGameRules::ClientDisconnected);
    SAnchor CPortalMPGameRules_SetMapCompleteData = PATCH_FUNC_ANCHOR(plugin.server_module(), CPortalMPGameRules::SetMapCompleteData);

    SAnchor CPortal_Player_Spawn = PATCH_FUNC_ANCHOR(plugin.server_module(), CPortal_Player::Spawn);
    SAnchor CPortal_Player_ClientCommand = PATCH_FUNC_ANCHOR(plugin.server_module(), CPortal_Player::ClientCommand);
    SAnchor CPortal_Player_OnFullyConnected = PATCH_FUNC_ANCHOR(plugin.server_module(), CPortal_Player::OnFullyConnected);
    //CPortal_Player::PlayerTransitionCompleteThink has already been patched, so it is no longer accessing m_bDataReceived

    UTIL_PlayerByIndex = (void *(*)(int)) PATCH_FUNC_ANCHOR(plugin.server_module(), UTIL_PlayerByIndex).get_addr();
    GetNumPlayersConnected = (int (*)()) PATCH_FUNC_ANCHOR(plugin.server_module(), GetNumPlayersConnected).get_addr();
    CPortalMPGameRules_SendAllMapCompleteData = (void (*)(void*)) PATCH_FUNC_ANCHOR(plugin.server_module(), CPortalMPGameRules::SendAllMapCompleteData).get_addr();
    CPortalMPGameRules_StartPlayerTransitionThinks = (void (*)(void*)) PATCH_FUNC_ANCHOR(plugin.server_module(), CPortalMPGameRules::StartPlayerTransitionThinks).get_addr();

    // - CPortalMPGameRules::~CPortalMPGameRules: detour to clear the connected list
    plugin.register_patch<CPatch>(CPortalMPGameRules_destr_CPortalMPGameRules + 0x5f, new SEQ_HEX("8b 9e a8 1a 00 00"),
        new SEQ_DETOUR_COPY_ORIG(plugin, 6, detour_CPortalMPGameRules_destr_CPortalMPGameRules, DETOUR_ARG_ESI)
    );

    // - CPortalMPGameRules::ClientCommandKeyValues: detour `m_bDataReceived[nPlayer] = true;` and `if(m_bDataReceived[0] && m_bDataReceived[1]) ...`
    plugin.register_patch<CPatch>(CPortalMPGameRules_ClientCommandKeyValues + 0xf8, new SEQ_HEX("c6 84 30 7d 1c 00 00 01"), 
        new SEQ_DETOUR(plugin, 8, detour_CPortalMPGameRules_ClientCommandKeyValues_A, DETOUR_ARG_EAX, DETOUR_ARG_EDI)
    );
    plugin.register_patch<CPatch>(CPortalMPGameRules_ClientCommandKeyValues + 0x23c, new SEQ_HEX("80 b8 7d 1c 00 00 00 74 09 80 b8 7e 1c 00 00 00"), new SEQ_SEQ(
        new SEQ_DETOUR(plugin, 14, detour_CPortalMPGameRules_ClientCommandKeyValues_B, DETOUR_ARG_EAX, DETOUR_ARG_EDX),
        new SEQ_HEX("85 d2") //test edx, edx
    ));

    // - CPortalMPGameRules::ClientDisconnected: detour to remove client from connected list
    //Replace the bIsSSCredits check to make room for the detour (it's no longer used because of the disconnect check patch anyway)
    plugin.register_patch<CPatch>(CPortalMPGameRules_ClientDisconnected + 0x69, new SEQ_MASKED_HEX("8b 0d ?? ?? ?? ?? 85 c9 74 0d e8 ?? ?? ?? ?? 84 c0 0f 85 b0 00 00 00"), new SEQ_SEQ(
        new SEQ_HEX("8b 4c 24 0c"), //mov ecx, [<this arg>]
        new SEQ_DETOUR(plugin, 19, detour_CPortalMPGameRules_ClientDisconnected, DETOUR_ARG_ECX, DETOUR_ARG_ESI)
    ));

    // - CPortalMPGameRules::SetMapCompleteData: detour `if(m_bDataReceived[nPlayer]) return;` and stub out the player find routine 
    //The nPlayer argument (originally determined by the player team) has been patched to be the raw player pointer itself 
    plugin.register_patch<CPatch>(CPortalMPGameRules_SetMapCompleteData + 0xf, new SEQ_HEX("83 f8 01 77 6e 8b 4d 08 80 bc 01 7d 1c 00 00 00"), new SEQ_SEQ(
        new SEQ_DETOUR(plugin, 14, detour_CPortalMPGameRules_SetMapCompleteData, DETOUR_ARG_LOCAL(8), DETOUR_ARG_EAX, DETOUR_ARG_EBX),
        new SEQ_HEX("85 db") //test ebx, ebx
    ));
    plugin.register_patch<CPatch>(CPortalMPGameRules_SetMapCompleteData + 0x40, new SEQ_MASKED_HEX("83 ec 0c 53 e8 ?? ?? ?? ?? 83 c4 10 89 c7 85 c0 74 23 8b 00 83 ec 0c 57 ff 90 58 01 00 00 83 c4 10 84 c0 74 10 83 ec 0c 57 e8 ?? ?? ?? ?? 83 c4 10 39 c6"), new SEQ_SEQ(
        new SEQ_HEX("89 c7 39 ff"), //mov edi, eax; cmp edi, edi
        new SEQ_FILL(47, 0x90)
    ));

    // - CPortal_Player::Spawn: detour `if(!m_bDataReceived[0] || !m_bDataReceived[1]) ...`
    plugin.register_patch<CPatch>(CPortal_Player_Spawn + 0xc95, new SEQ_HEX("80 b8 7d 1c 00 00 00 74 0d 80 b8 7e 1c 00 00 00"), new SEQ_SEQ(
        new SEQ_DETOUR(plugin, 14, detour_CPortal_Player_Spawn, DETOUR_ARG_EAX, DETOUR_ARG_EDX),
        new SEQ_HEX("85 d2") //test edx, edx
    ));

    // - CPortal_Player::ClientCommand: replace the SetMapCompleteData nPlayer argument with the player pointer
    plugin.register_patch<CPatch>(CPortal_Player_ClientCommand + 0xbed, new SEQ_MASKED_HEX("e8 ?? ?? ?? ??"), new SEQ_HEX("8b 45 08 90 90"));

    // - CPortal_Player::OnFullyConnected: detour `if(!m_bDataReceived[0] || !m_bDataReceived[1]) ...`
    plugin.register_patch<CPatch>(CPortal_Player_OnFullyConnected + 0x9d, new SEQ_HEX("80 b8 7d 1c 00 00 00 0f 84 36 01 00 00 80 b8 7e 1c 00 00 00"), new SEQ_SEQ(
        new SEQ_DETOUR(plugin, 18, detour_CPortal_Player_OnFullyConnected, DETOUR_ARG_EAX, DETOUR_ARG_EDI),
        new SEQ_HEX("85 ff") //test edi, edi
    ));
}

uint16_t& CTransitionsFixPatch::get_rules_slot_list(void *rules) {
    return *(uint16_t*) ((uint8_t*) rules + OFF_CPortalMPGameRules_m_bDataReceived);
}

bool CTransitionsFixPatch::is_everyone_ready(void *rules, void *ignore_player) {
    int player_count = GetNumPlayersConnected();
    int list_player_count = player_slots.num_players_in_list(get_rules_slot_list(rules));

    if(ignore_player) {
        //Check if the player appears in the client list
        for(int i = 1; i <= MAXCLIENTS; i++) {
            if(UTIL_PlayerByIndex(i) == ignore_player) {
                DevMsg("CTransitionsFixPatch | Ignoring CPortal_Player %p for CPortalMPGameRules %p ready check\n", ignore_player, rules);
                player_count--;
                break;
            }
        }
    }

    DevMsg("CTransitionsFixPatch | CPortalMPGameRules %p ready check: %d / %d players in list / total\n", rules, list_player_count, player_count);
    return list_player_count >= player_count;
}

DETOUR_FUNC void CTransitionsFixPatch::detour_CPortalMPGameRules_destr_CPortalMPGameRules(void **ptr_rules) {
    void *rules = *ptr_rules;
    DevMsg("DETOUR CTransitionsFixPatch | CPortalMPGameRules::~CPortalMPGameRules | this=%p\n", rules);

    //Free the slot list
    Msg("P2MPPatch | Freeing ready player list for CPortalMPGameRules %p\n", ptr_rules);
    player_slots.free_slot_list(get_rules_slot_list(rules));
}

DETOUR_FUNC void CTransitionsFixPatch::detour_CPortalMPGameRules_ClientCommandKeyValues_A(void **ptr_rules, void **ptr_pPlayer) {
    void *rules = *ptr_rules;
    void *pPlayer = *ptr_pPlayer;
    DevMsg("DETOUR CTransitionsFixPatch | CPortalMPGameRules::ClientCommandKeyValues A | this=%p pPlayer=%p\n", rules, pPlayer);

    //Add the player to the slot list
    if(!player_slots.list_contains_player(get_rules_slot_list(rules), pPlayer)) {
        player_slots.add_player_to_list(get_rules_slot_list(rules), pPlayer);
        Msg("P2MPPatch | Added CPortal_Player %p to ready list for CPortalMPGameRules %p, new player count: %d\n", pPlayer, rules, player_slots.num_players_in_list(get_rules_slot_list(rules)));
    }
}

DETOUR_FUNC void CTransitionsFixPatch::detour_CPortalMPGameRules_ClientCommandKeyValues_B(void **ptr_rules, int *ptr_everyoneReady) {
    void *rules = *ptr_rules;
    DevMsg("DETOUR CTransitionsFixPatch | CPortalMPGameRules::ClientCommandKeyValues B | this=%p\n", rules);

    *ptr_everyoneReady = is_everyone_ready(rules);
    if(*ptr_everyoneReady) Msg("P2MPPatch | All players ready, ending transition...\n");
}

DETOUR_FUNC void CTransitionsFixPatch::detour_CPortalMPGameRules_ClientDisconnected(void **ptr_rules, void **ptr_pPlayer) {
    void *rules = *ptr_rules;
    void *pPlayer = *ptr_pPlayer;
    DevMsg("DETOUR CTransitionsFixPatch | CPortalMPGameRules::ClientDisconnected | this=%p pPlayer=%p\n", rules, pPlayer);

    //Remove the player from the slot list
    if(player_slots.remove_player_from_list(get_rules_slot_list(rules), pPlayer)) {
        Msg("P2MPPatch | Removed CPortal_Player %p from ready list for CPortalMPGameRules %p, new player count: %d\n", pPlayer, rules, player_slots.num_players_in_list(get_rules_slot_list(rules)));
    } else if(is_everyone_ready(rules, pPlayer)) {
        Msg("P2MPPatch | All players ready after CPortal_Player %p disconnected, ending transition...\n", pPlayer);
        CPortalMPGameRules_SendAllMapCompleteData(rules);
        CPortalMPGameRules_StartPlayerTransitionThinks(rules);
    }
}

DETOUR_FUNC void CTransitionsFixPatch::detour_CPortalMPGameRules_SetMapCompleteData(void **ptr_rules, void **ptr_pPlayer, int *ptr_shouldAbort) {
    void *rules = *ptr_rules;
    void *pPlayer = *ptr_pPlayer;
    DevMsg("DETOUR CTransitionsFixPatch | CPortalMPGameRules::SetMapCompleteData | this=%p pPlayer=%p\n", rules, pPlayer);

    *ptr_shouldAbort = player_slots.list_contains_player(get_rules_slot_list(rules), pPlayer);
    DevMsg(*ptr_shouldAbort ? " -> ready list contains the player, aborting...\n" : " -> ready list does not contain the player, continuing...\n");
}

DETOUR_FUNC void CTransitionsFixPatch::detour_CPortal_Player_Spawn(void **ptr_rules, int *ptr_everyoneReady) {
    void *rules = *ptr_rules;
    DevMsg("DETOUR CTransitionsFixPatch | CPortal_Player::Spawn | rules=%p\n", rules);
    *ptr_everyoneReady = is_everyone_ready(rules);
}

DETOUR_FUNC void CTransitionsFixPatch::detour_CPortal_Player_OnFullyConnected(void **ptr_rules, int *ptr_everyoneReady) {
    void *rules = *ptr_rules;
    DevMsg("DETOUR CTransitionsFixPatch | CPortal_Player::OnFullyConnected | rules=%p\n", rules);
    *ptr_everyoneReady = is_everyone_ready(rules);
}