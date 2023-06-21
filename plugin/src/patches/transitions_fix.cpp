#include <tier0/dbg.h>
#include <iserver.h>
#include <tier0/valve_minmax_off.h>

#include "byteseq.hpp"
#include "plugin.hpp"
#include "transitions_fix.hpp"
#include "anchors.hpp"
#include "vtab.hpp"

using namespace patches;

CTransitionsFixPatch::SReadyTracker *CTransitionsFixPatch::tracker_slots[NUM_TRACKER_SLOTS];

int CTransitionsFixPatch::OFF_CPortalMPGameRules_m_bDataReceived;

IServer *CTransitionsFixPatch::glob_sv;
void **CTransitionsFixPatch::ptr_g_pMatchFramework;

void *(*CTransitionsFixPatch::UTIL_PlayerByIndex)(int);
int (*CTransitionsFixPatch::KeyValues_GetInt)(void*, const char*, int);
void (*CTransitionsFixPatch::CPortalMPGameRules_SendAllMapCompleteData)(void*);
void (*CTransitionsFixPatch::CPortalMPGameRules_StartPlayerTransitionThinks)(void*);

void CTransitionsFixPatch::register_patches(CMPPatchPlugin& plugin) {
    OFF_CPortalMPGameRules_m_bDataReceived = anchors::server::CPortalMPGameRules::m_bDataReceived.get(plugin.server_module());

    //Find CPortalMPGameRules::CPortalMPGameRules and set m_bMapNamesLoaded to true by default
    //This causes the level_complete_data -> read_stats -> StartPlayerTransitionThinks cascade to happen for SP maps as well
    SAnchor CPortalMPGameRules_CPortalMPGameRules = anchors::server::CPortalMPGameRules::CPortalMPGameRules.get(plugin.server_module());
    plugin.register_patch<CSeqPatch>(CPortalMPGameRules_CPortalMPGameRules + 0x3aa, new SEQ_HEX("80 bf bc 1b 00 00 00 0f 85 59 01 00 00"), new SEQ_HEX("c6 87 7c 02 00 00 01 90 90 90 90 90 90"));

    //Rewire the functionality of m_bDataReceived
    //This bool[2] array is used to track whether all players are ready during a transition
    //Replace it with an arbitrary-size ready tracker, reusing the 2 bytes as the uint16_t slot index

    // - find functions/globals we need to link to
    glob_sv = (IServer*) anchors::engine::sv.get(plugin.engine_module()).get_addr();
    ptr_g_pMatchFramework = (void**) anchors::server::g_pMatchFramework.get(plugin.server_module()).get_addr();

    UTIL_PlayerByIndex = (void *(*)(int)) anchors::server::UTIL_PlayerByIndex.get(plugin.server_module()).get_addr();
    KeyValues_GetInt = (int (*)(void*, const char*, int)) anchors::engine::KeyValues::GetInt.get(plugin.engine_module()).get_addr();
    CPortalMPGameRules_SendAllMapCompleteData = (void (*)(void*)) anchors::server::CPortalMPGameRules::SendAllMapCompleteData.get(plugin.server_module()).get_addr();
    CPortalMPGameRules_StartPlayerTransitionThinks = (void (*)(void*)) anchors::server::CPortalMPGameRules::StartPlayerTransitionThinks.get(plugin.server_module()).get_addr();

    // - CPortalMPGameRules::CPortalMPGameRules: hook to allocate the ready tracker
    CHookTracker& htrack_CPortalMPGameRules_CPortalMPGameRules = anchors::server::CPortalMPGameRules::CPortalMPGameRules.hook_tracker(plugin.server_module()); 
    plugin.register_patch<CFuncHook>(plugin.scratchpad(), htrack_CPortalMPGameRules_CPortalMPGameRules, (void*) &hook_CPortalMPGameRules_CPortalMPGameRules);

    // - CPortalMPGameRules::~CPortalMPGameRules: hook to delete the ready tracker
    CHookTracker& htrack_CPortalMPGameRules_destr_CPortalMPGameRules = anchors::server::CPortalMPGameRules::destr_CPortalMPGameRules.hook_tracker(plugin.server_module()); 
    plugin.register_patch<CFuncHook>(plugin.scratchpad(), htrack_CPortalMPGameRules_destr_CPortalMPGameRules, (void*) &hook_CPortalMPGameRules_destr_CPortalMPGameRules);

    // - CPortalMPGameRules::ClientCommandKeyValues: detour `m_bDataReceived[nPlayer] = true;` and `if(m_bDataReceived[0] && m_bDataReceived[1]) ...`
    SAnchor CPortalMPGameRules_ClientCommandKeyValues = anchors::server::CPortalMPGameRules::ClientCommandKeyValues.get(plugin.server_module());
    plugin.register_patch<CSeqPatch>(CPortalMPGameRules_ClientCommandKeyValues + 0xf8, new SEQ_HEX("c6 84 30 7d 1c 00 00 01"), 
        new SEQ_FUNC_DETOUR(plugin, 8, detour_CPortalMPGameRules_ClientCommandKeyValues_A, DETOUR_ARG_EAX, DETOUR_ARG_EDI)
    );
    plugin.register_patch<CSeqPatch>(CPortalMPGameRules_ClientCommandKeyValues + 0x23c, new SEQ_HEX("80 b8 7d 1c 00 00 00 74 09 80 b8 7e 1c 00 00 00"), new SEQ_SEQ(
        new SEQ_FUNC_DETOUR(plugin, 14, detour_CPortalMPGameRules_ClientCommandKeyValues_B, DETOUR_ARG_EAX, DETOUR_ARG_EDX),
        new SEQ_HEX("85 d2") //test edx, edx
    ));

    // - CPortalMPGameRules::ClientDisconnected: detour to remove the player from the ready tracker
    //Replace the bIsSSCredits check to make room for the detour (it's no longer used because of the disconnect check patch anyway)
    SAnchor CPortalMPGameRules_ClientDisconnected = anchors::server::CPortalMPGameRules::ClientDisconnected.get(plugin.server_module());
    plugin.register_patch<CSeqPatch>(CPortalMPGameRules_ClientDisconnected + 0x69, new SEQ_MASKED_HEX("8b 0d ?? ?? ?? ?? 85 c9 74 0d e8 ?? ?? ?? ?? 84 c0 0f 85 b0 00 00 00"), new SEQ_SEQ(
        new SEQ_HEX("8b 4c 24 0c"), //mov ecx, [<this arg>]
        new SEQ_FUNC_DETOUR(plugin, 19, detour_CPortalMPGameRules_ClientDisconnected, DETOUR_ARG_ECX, DETOUR_ARG_ESI)
    ));

    // - CPortalMPGameRules::SetMapCompleteData: detour `if(m_bDataReceived[nPlayer]) return;` and stub out the player find routine 
    //The nPlayer argument (originally determined by the player team) has been patched to be the raw player pointer itself
    SAnchor CPortalMPGameRules_SetMapCompleteData = anchors::server::CPortalMPGameRules::SetMapCompleteData.get(plugin.server_module());
    plugin.register_patch<CSeqPatch>(CPortalMPGameRules_SetMapCompleteData + 0xf, new SEQ_HEX("83 f8 01 77 6e 8b 4d 08 80 bc 01 7d 1c 00 00 00"), new SEQ_SEQ(
        new SEQ_FUNC_DETOUR(plugin, 14, detour_CPortalMPGameRules_SetMapCompleteData, DETOUR_ARG_LOCAL(8), DETOUR_ARG_EAX, DETOUR_ARG_EBX),
        new SEQ_HEX("85 db") //test ebx, ebx
    ));
    plugin.register_patch<CSeqPatch>(CPortalMPGameRules_SetMapCompleteData + 0x40, new SEQ_MASKED_HEX("83 ec 0c 53 e8 ?? ?? ?? ?? 83 c4 10 89 c7 85 c0 74 23 8b 00 83 ec 0c 57 ff 90 58 01 00 00 83 c4 10 84 c0 74 10 83 ec 0c 57 e8 ?? ?? ?? ?? 83 c4 10 39 c6"), new SEQ_SEQ(
        new SEQ_HEX("89 c7 39 ff"), //mov edi, eax; cmp edi, edi
        new SEQ_FILL(47, 0x90)
    ));

    // - CPortal_Player::Spawn: detour `if(!m_bDataReceived[0] || !m_bDataReceived[1]) ...`
    SAnchor CPortal_Player_Spawn = anchors::server::CPortal_Player::Spawn.get(plugin.server_module());
    plugin.register_patch<CSeqPatch>(CPortal_Player_Spawn + 0xc95, new SEQ_HEX("80 b8 7d 1c 00 00 00 74 0d 80 b8 7e 1c 00 00 00"), new SEQ_SEQ(
        new SEQ_FUNC_DETOUR(plugin, 14, detour_CPortal_Player_Spawn, DETOUR_ARG_EAX, DETOUR_ARG_EDX),
        new SEQ_HEX("85 d2") //test edx, edx
    ));

    // - CPortal_Player::ClientCommand: replace the SetMapCompleteData nPlayer argument with the player pointer
    SAnchor CPortal_Player_ClientCommand = anchors::server::CPortal_Player::ClientCommand.get(plugin.server_module());
    plugin.register_patch<CSeqPatch>(CPortal_Player_ClientCommand + 0xbed, new SEQ_MASKED_HEX("e8 ?? ?? ?? ??"), new SEQ_HEX("8b 45 08 90 90"));

    // - CPortal_Player::OnFullyConnected: detour `if(!m_bDataReceived[0] || !m_bDataReceived[1]) ...`
    SAnchor CPortal_Player_OnFullyConnected = anchors::server::CPortal_Player::OnFullyConnected.get(plugin.server_module());
    plugin.register_patch<CSeqPatch>(CPortal_Player_OnFullyConnected + 0x9d, new SEQ_HEX("80 b8 7d 1c 00 00 00 0f 84 36 01 00 00 80 b8 7e 1c 00 00 00"), new SEQ_SEQ(
        new SEQ_FUNC_DETOUR(plugin, 18, detour_CPortal_Player_OnFullyConnected, DETOUR_ARG_EAX, DETOUR_ARG_EDI),
        new SEQ_HEX("85 ff") //test edi, edi
    ));

    // - CPortal_Player::PlayerTransitionCompleteThink has already been patched, so it is no longer accessing m_bDataReceived
}

void CTransitionsFixPatch::SReadyTracker::init_match_req_player_count() {
    req_player_cnt = -1;

    //Access g_pMatchFramework->GetMatchSession()->GetSessionSettings()->GetInt("members/numPlayers") if it exists
    void *g_pMatchFramework = *ptr_g_pMatchFramework;
    if(!g_pMatchFramework) {
        DevMsg("CTransitionsFixPatch | g_pMatchFramework = nullptr\n");
        return;
    }

    void *match_session = PATCH_VTAB_FUNC(g_pMatchFramework, matchmaking::IMatchFramework::GetMatchSession)(g_pMatchFramework);
    if(!match_session) {
        DevMsg("CTransitionsFixPatch | g_pMatchFramework->GetMatchSession() = nullptr\n");
        return;
    }

    void *session_settings = PATCH_VTAB_FUNC(match_session, matchmaking::IMatchSession::GetSessionSettings)(match_session);
    if(!session_settings) {
        DevMsg("CTransitionsFixPatch | g_pMatchFramework->GetMatchSession()->GetSessionSettings() = nullptr\n");
        return;
    }

    req_player_cnt = KeyValues_GetInt(session_settings, "members/numPlayers", -1);
    DevMsg("CTransitionsFixPatch | g_pMatchFramework->GetMatchSession()->GetSessionSettings()->GetInt(\"members/numPlayers\") = %d\n", req_player_cnt);
}

int CTransitionsFixPatch::SReadyTracker::get_req_players() const {
    if(req_player_cnt >= 0) return req_player_cnt;
    return glob_sv->GetNumClients() - glob_sv->GetNumProxies();
}

bool CTransitionsFixPatch::SReadyTracker::is_everyone_ready() const {
    if(is_ready) return true;

    int req_players = get_req_players();
    DevMsg("CTransitionsFixPatch | SReadyTracker %p ready check: %d / %d players in list / total\n", this, ready_players.size(), req_players);
    return ready_players.size() >= get_req_players();
}

HOOK_FUNC void CTransitionsFixPatch::hook_CPortalMPGameRules_CPortalMPGameRules(HOOK_ORIG void (*orig)(void*), void *rules) {
    DevMsg("HOOK CTransitionsFixPatch | CPortalMPGameRules::CPortalMPGameRules | this=%p\n", rules);

    orig(rules);

    //Find a free tracker slot for the rules to reference
    int slot = -1;
    for(int i = 0; i < NUM_TRACKER_SLOTS; i++) {
        if(tracker_slots[i]) continue;
        slot = i;
        break;
    }
    if(slot < 0) abort();

    //Allocate a ready tracker
    SReadyTracker *tracker = new SReadyTracker();
    tracker_slots[slot] = tracker;
    get_rules_ready_tracker_slot(rules) = slot;

    Msg("P2MPPatch | Allocated SReadyTracker %p [slot %d] for CPortalMPGameRules %p\n", tracker_slots[slot], slot, rules);

    //Check if this is the initial load
    //If so, obtain the number of initial players from the matchmaking server
    if(tracker->get_req_players() == 0) {
        tracker->init_match_req_player_count();
        if(tracker->req_player_cnt >= 0) Msg("P2MPPatch | Detected initial server start, requested player count from matchmaking server -> %d players\n", tracker->req_player_cnt);
        else Warning("P2MPPatch | Detected initial server start, but failed to request player count from matchmaking server\n");
    }
}

HOOK_FUNC void CTransitionsFixPatch::hook_CPortalMPGameRules_destr_CPortalMPGameRules(HOOK_ORIG void (*orig)(void*), void *rules) {
    DevMsg("HOOK CTransitionsFixPatch | CPortalMPGameRules::~CPortalMPGameRules | this=%p\n", rules);

    //Free the ready tracker
    Msg("P2MPPatch | Freeing SReadyTracker %p [slot %d] for CPortalMPGameRules %p\n", get_rules_ready_tracker(rules), (int) get_rules_ready_tracker_slot(rules), rules);
    delete get_rules_ready_tracker(rules);
    tracker_slots[get_rules_ready_tracker_slot(rules)] = nullptr;

    orig(rules);
}

DETOUR_FUNC void CTransitionsFixPatch::detour_CPortalMPGameRules_ClientCommandKeyValues_A(void **ptr_rules, void **ptr_pPlayer) {
    void *rules = *ptr_rules;
    void *pPlayer = *ptr_pPlayer;
    DevMsg("DETOUR CTransitionsFixPatch | CPortalMPGameRules::ClientCommandKeyValues A | this=%p pPlayer=%p\n", rules, pPlayer);

    //Add the player to the ready tracker
    SReadyTracker *tracker = get_rules_ready_tracker(rules);
    if(tracker->ready_players.insert(pPlayer).second) {
        Msg("P2MPPatch | Added CPortal_Player %p to CPortalMPGameRules %p ready tracker, new player count: %d\n", pPlayer, rules, tracker->ready_players.size());
    }
}

DETOUR_FUNC void CTransitionsFixPatch::detour_CPortalMPGameRules_ClientCommandKeyValues_B(void **ptr_rules, int *ptr_endTransition) {
    void *rules = *ptr_rules;
    DevMsg("DETOUR CTransitionsFixPatch | CPortalMPGameRules::ClientCommandKeyValues B | this=%p\n", rules);

    SReadyTracker *tracker = get_rules_ready_tracker(rules);

    if(tracker->is_ready) {
        //We have already started, so just send the map complete data again as it has been modified
        Msg("P2MPPatch | Resending map complete data for CPortalMPGameRules %p after new player joined\n", rules);
        CPortalMPGameRules_SendAllMapCompleteData(rules);
        *ptr_endTransition = false;
        return;
    }

    //Check if everyone is ready now
    //This check is special, as it determines if the game ends the transition
    if(tracker->is_everyone_ready()) {
        Msg("P2MPPatch | All players ready, ending transition for CPortalMPGameRules %p...\n", rules);
        tracker->is_ready = true;
    }

    *ptr_endTransition = tracker->is_ready;
}

DETOUR_FUNC void CTransitionsFixPatch::detour_CPortalMPGameRules_ClientDisconnected(void **ptr_rules, void **ptr_pPlayer) {
    void *rules = *ptr_rules;
    void *pPlayer = *ptr_pPlayer;
    DevMsg("DETOUR CTransitionsFixPatch | CPortalMPGameRules::ClientDisconnected | this=%p pPlayer=%p\n", rules, pPlayer);

    if(!pPlayer) return; //Apparently this is a valid set of arguments

    SReadyTracker *tracker = get_rules_ready_tracker(rules);

    //Remove the player from the ready tracker
    bool didRemovePlayer = tracker->ready_players.erase(pPlayer) > 0;

    //Skip the following logic if we're already ready
    if(tracker->is_ready) return; 

    if(didRemovePlayer) {
        //We succesfully removed the player
        Msg("P2MPPatch | Removed CPortal_Player %p from CPortalMPGameRules %p ready tracker, new player count: %d / %d\n", pPlayer, rules, tracker->ready_players.size(), tracker->get_req_players());
        return;
    }

    //The player wasn't ready before disconnecting
    //Now that they are no longer stalling the transition, check if everyone else is ready
    if(tracker->is_everyone_ready()) {
        Msg("P2MPPatch | All players ready after CPortal_Player %p disconnected, ending transition for CPortalMPGameRules %p...\n", pPlayer, rules);
        tracker->is_ready = true;

        //We have to manually end the transition, as the usual ClientCommandKeyValues logic isn't run
        CPortalMPGameRules_SendAllMapCompleteData(rules);
        CPortalMPGameRules_StartPlayerTransitionThinks(rules);
    }
}

DETOUR_FUNC void CTransitionsFixPatch::detour_CPortalMPGameRules_SetMapCompleteData(void **ptr_rules, void **ptr_pPlayer, int *ptr_shouldAbort) {
    void *rules = *ptr_rules;
    void *pPlayer = *ptr_pPlayer;
    DevMsg("DETOUR CTransitionsFixPatch | CPortalMPGameRules::SetMapCompleteData | this=%p pPlayer=%p\n", rules, pPlayer);

    SReadyTracker *tracker = get_rules_ready_tracker(rules);

    //Check if the player is already ready
    *ptr_shouldAbort = (tracker->ready_players.find(pPlayer) != tracker->ready_players.end());
    if(*ptr_shouldAbort) DevMsg("CTransitionsFixPatch | CPortal_Player %p already in SReadyTracker %p, aborting 'read_stats' request\n", pPlayer, tracker);
    else DevMsg("CTransitionsFixPatch | CPortal_Player %p not in SReadyTracker %p, sending 'read_stats' request...\n", pPlayer, tracker);
}

DETOUR_FUNC void CTransitionsFixPatch::detour_CPortal_Player_Spawn(void **ptr_rules, int *ptr_everyoneReady) {
    void *rules = *ptr_rules;
    DevMsg("DETOUR CTransitionsFixPatch | CPortal_Player::Spawn | rules=%p\n", rules);
    *ptr_everyoneReady = get_rules_ready_tracker(rules)->is_everyone_ready();
}

DETOUR_FUNC void CTransitionsFixPatch::detour_CPortal_Player_OnFullyConnected(void **ptr_rules, int *ptr_everyoneReady) {
    void *rules = *ptr_rules;
    DevMsg("DETOUR CTransitionsFixPatch | CPortal_Player::OnFullyConnected | rules=%p\n", rules);
    *ptr_everyoneReady = get_rules_ready_tracker(rules)->is_everyone_ready();
}