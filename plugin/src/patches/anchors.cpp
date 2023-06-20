#include "anchors.hpp"

using namespace patches::anchors;

template<typename T> T SRefInstrAnchor<T>::determine_value(CModule& module) const {
    //Check the reference instruction
    SAnchor ref_instr = ref_instr_anchor(module);
    uint8_t *ref_instr_ptr = (uint8_t*) ref_instr.get_addr();
    if(m_RefInstrSequence.compare(ref_instr_ptr, 0, m_RefInstrSequence.size()) != 0) {
        DevWarning("Mismatching global variable '%s' anchor instruction %s: ", m_Name, ref_instr.debug_str().c_str());
        for(size_t i = 0; i < m_RefInstrSequence.size(); i++) DevWarning("%02x", m_RefInstrSequence[i]);
        DevWarning(" != ");
        for(size_t i = 0; i < m_RefInstrSequence.size(); i++) DevWarning("%02x", ref_instr_ptr[i]);
        DevWarning("\n");

        std::stringstream sstream;
        sstream << "Global variable anchor instruction " << ref_instr << " doesn't match target [" << m_RefInstrSequence.size() << " bytes]";
        throw std::runtime_error(sstream.str());
    }

    return *(T*) (ref_instr_ptr + m_RefInstrValOff);
}

template uint8_t SRefInstrAnchor<uint8_t>::determine_value(CModule& module) const;
template uint16_t SRefInstrAnchor<uint16_t>::determine_value(CModule& module) const;
template int SRefInstrAnchor<int>::determine_value(CModule& module) const;
template void *SRefInstrAnchor<void*>::determine_value(CModule& module) const;

namespace patches::anchors {

    //>>>>> server anchors <<<<<

    namespace server {
        SGlobVarAnchor g_pGameRules("g_pGameRules", CProp_Portal::DispatchPortalPlacementParticles, 0x9, "a1 ?? ?? ?? ??", 1);
        SGlobVarAnchor g_pMatchFramework("g_pMatchFramework", CProp_Portal::Spawn, 0x4c, "a1 ?? ?? ?? ??", 1);

        SFuncAnchor UTIL_PlayerByIndex("UTIL_PlayerByIndex", "39 50 14 7c 2e 8b 40 58 85 c0 74 22 c1 e2 04 01 d0 f6 00 02", 0xd);

        SFuncAnchor FUNC_numSlots_adj("FUNC_numSlots_adj", "83 ec 0c 8b 10 50 ff 52 10 83 c4 10 89 c7 85 c0 74 12", 0x75);

        namespace CServerGameClients {
            SFuncAnchor GetPlayerLimits("CServerGameClients::GetPlayerLimits", "83 ec 0c 8b 44 24 1c c7 00 01 00 00 00 8b 44 24 14 c7 00 01 00 00 00");
        }

        namespace CPortalMPGameRules {
            SMemberOffAnchor m_bDataReceived("CPortalMPGameRules::m_bDataReceived", CPortalMPGameRules, 0x316, "66 89 8f ?? ?? ?? ??", 3);

            SFuncAnchor CPortalMPGameRules("CPortalMPGameRules::CPortalMPGameRules", "83 c4 10 c7 87 30 02 00 00 00 00 00 00", 0x1a);
            SFuncAnchor destr_CPortalMPGameRules("CPortalMPGameRules::~CPortalMPGameRules", "89 d8 c1 e0 04 03 86 9c 1a 00 00 8b 50 08 c7 40 0c 00 00 00 00 85 d2", 0x70);

            SFuncAnchor ClientCommandKeyValues("CPortalMPGameRules::ClientCommandKeyValues", "55 57 56 53 83 ec 3c 8b 44 24 50 8b 5c 24 58 89 44 24 04 8b 44 24 54 85 db");
            SFuncAnchor ClientDisconnected("CPortalMPGameRules::ClientDisconnected", "55 57 56 53 83 ec 28 8b 44 24 3c 8b 7c 24 40 89 44 24 18 57");

            SFuncAnchor SetMapCompleteData("CPortalMPGameRules::SetMapCompleteData", "55 89 e5 57 56 53 81 ec 2c 01 00 00 8b 45 0c 83 f8 01");
            SFuncAnchor SendAllMapCompleteData("CPortalMPGameRules::SendAllMapCompleteData", "31 c0 b9 08 00 00 00 83 c4 10 8d 7d c8 ba 01 00 00 00 f3 ab 8b 45 08", 0x41);
            SFuncAnchor StartPlayerTransitionThinks("CPortalMPGameRules::StartPlayerTransitionThinks", "83 ec 08 be 41 08 00 00 31 ff 6a 00 6a 00 57 56 50 8d 44 24 24 50", 0x36);
        }

        namespace CGameMovement {
            SMemberOffAnchor m_LastStuck("CGameMovement::m_LastStuck", CheckStuck, 0x126, "8b 88 ?? ?? ?? ??", 2);
            SFuncAnchor CheckStuck("CGameMovement::CheckStuck", "8d 6c 24 30 83 ec 0c 8b 13 c6 83 6e 06 00 00 01 8d 7c 24 48 57", 0x1f);
        }

        SFuncAnchor ServerClassInit_DT_BaseEntity("ServerClassInit<DT_BaseEntity>", "83 c4 1c 6a 20 6a 00 6a 01 6a 05 6a 04 68 c0 02 00 00", 0xb5);
        namespace CBaseEntity {
            SMemberOffAnchor m_MoveType("CBaseEntity::m_MoveType", ServerClassInit_DT_BaseEntity, 0x3ba, "68 ?? ?? ?? ??", 1);

            SFuncAnchor ThinkSet("CBaseEntity::ThinkSet", "55 57 56 53 83 ec 1c 8b 74 24 44 f3 0f 7e 44 24 38");
            SFuncAnchor SetNextThink("CBaseEntity::SetNextThink", "f3 0f 10 44 24 34 8b 6c 24 30 0f 2e c1 8b 74 24 38", 0x7);
        }

        namespace CPortal_Player {
            SFuncAnchor Spawn("CPortal_Player::Spawn", "8b 03 89 1c 24 ff 90 3c 08 00 00 83 c4 10 80 bb 10 0b 00 00 00", 0x62);
            SFuncAnchor ShouldCollide("CPortal_Player::ShouldCollide", "53 8b 4c 24 08 8b 44 24 0c 8b 5c 24 10 8b 52 30", 0x6);

            SFuncAnchor ClientCommand("CPortal_Player::ClientCommand", "55 89 e5 57 56 53 81 ec 9c 00 00 00 8b 7d 0c 8b 5d 08 8b 07 85 c0");

            SFuncAnchor OnFullyConnected("CPortal_Player::OnFullyConnected", "8b 5c 24 4c 8b 10 50 ff 92 88 00 00 00 83 c4 10 84 c0", 0xc);

            SFuncAnchor PlayerTransitionCompleteThink("CPortal_Player::PlayerTransitionCompleteThink", "8b 5c 24 28 8b 10 68 00 00 f8 3f 6a 00", 0xa);
        }

        namespace CProp_Portal {
            SFuncAnchor Spawn("CProp_Portal::Spawn", "53 83 ec 14 8b 5c 24 1c 8b 03 53 ff 50 68 89 1c 24");
            SFuncAnchor DispatchPortalPlacementParticles("CProp_Portal::DispatchPortalPlacementParticles", "8b 5d 0c 8b 10 50 ff 92 88 00 00 00 83 c4 10 84 c0", 0xe);
        }

        namespace CEnvFade {
            SFuncAnchor InputFade("CEnvFade::InputFade", "89 c2 83 e2 01 29 d3 89 da 83 ca 04 a8 02 0f 45 da 89 da 83 ca 08 a8 08 0f 45 da a8 04", 0x16);
            SFuncAnchor InputReverseFade("CEnvFade::InputReverseFade", "89 c6 83 e6 01 83 c6 01 89 f2 83 ca 04 a8 02 0f 45 f2 89 f2 83 ca 08 a8 08 0f 45 f2", 0x20);
        }
    }

    //>>>>> matchmaking anchors <<<<<

    namespace matchmaking {
        namespace CMatchTitleGameSettingsMgr {
            SFuncAnchor InitializeGameSettings("CMatchTitleGameSettingsMgr::InitializeGameSettings", "57 56 53 8b 5c 24 14 8b 74 24 18 83 ec 04");
        }
    }

    //>>>>> engine anchors <<<<<

    namespace engine {    
        SGlobVarAnchor sv("sv", SV_Frame, 0x4c, "68 ?? ?? ?? ??", 1);

        SFuncAnchor SV_Frame("SV_Frame", "85 c0 74 14 89 f1 84 c9 74 0e 8b 10 83 ec 08 6a 01 50", 0x1f);

        namespace CGameServer {
            SFuncAnchor InitMaxClients("CGameServer::InitMaxClients", "8b 5c 24 20 c7 44 24 04 01 00 00 00 c7 44 24 08 40 00 00 00", 0xa);
        }

        namespace KeyValues {
            SFuncAnchor GetInt("KeyValues::GetInt", "83 c4 10 85 c0 74 1a 0f b6 50 10 80 fa 05 74 59 7f 17 80 fa 01 74 2a", 0x17);
        }
    }
}