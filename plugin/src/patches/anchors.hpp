#ifndef H_P2MPPATCH_PATCHES_ANCHORS
#define H_P2MPPATCH_PATCHES_ANCHORS

#include <tier0/dbg.h>
#include <tier0/valve_minmax_off.h>
#include "byteseq.hpp"
#include "patch.hpp"

namespace patches {
    namespace anchors {
        struct SFuncAnchor {
            CHexSequence sequence;
            int offset;

            SFuncAnchor(const char *hex_seq, int off = 0) : sequence(hex_seq), offset(off) {}
        };

        #define PATCH_FUNC_ANCHOR(module, name) ({\
            SAnchor __func_anchor = module.find_seq_anchor(patches::anchors::name.sequence) - patches::anchors::name.offset;\
            DevMsg("&(" #name ") = %p\n", __func_anchor.get_addr());\
            __func_anchor;\
        })

        //>>>>> server anchors <<<<<

        const SFuncAnchor UTIL_PlayerByIndex("39 50 14 7c 2e 8b 40 58 85 c0 74 22 c1 e2 04 01 d0 f6 00 02", 0xd);

        const SFuncAnchor FUNC_numSlots_adj("83 ec 0c 8b 10 50 ff 52 10 83 c4 10 89 c7 85 c0 74 12", 0x75);

        namespace CServerGameClients {
            const SFuncAnchor GetPlayerLimits("83 ec 0c 8b 44 24 1c c7 00 01 00 00 00 8b 44 24 14 c7 00 01 00 00 00");
        }

        namespace CPortalMPGameRules {
            const SFuncAnchor CPortalMPGameRules("83 c4 10 c7 87 30 02 00 00 00 00 00 00", 0x1a);
            const SFuncAnchor destr_CPortalMPGameRules("89 d8 c1 e0 04 03 86 9c 1a 00 00 8b 50 08 c7 40 0c 00 00 00 00 85 d2", 0x70);

            const SFuncAnchor ClientCommandKeyValues("55 57 56 53 83 ec 3c 8b 44 24 50 8b 5c 24 58 89 44 24 04 8b 44 24 54 85 db");
            const SFuncAnchor ClientDisconnected("55 57 56 53 83 ec 28 8b 44 24 3c 8b 7c 24 40 89 44 24 18 57");

            const SFuncAnchor SetMapCompleteData("55 89 e5 57 56 53 81 ec 2c 01 00 00 8b 45 0c 83 f8 01");
            const SFuncAnchor SendAllMapCompleteData("31 c0 b9 08 00 00 00 83 c4 10 8d 7d c8 ba 01 00 00 00 f3 ab 8b 45 08", 0x41);
            const SFuncAnchor StartPlayerTransitionThinks("83 ec 08 be 41 08 00 00 31 ff 6a 00 6a 00 57 56 50 8d 44 24 24 50", 0x36);
        }

        namespace CBaseEntity {
            const SFuncAnchor ThinkSet("55 57 56 53 83 ec 1c 8b 74 24 44 f3 0f 7e 44 24 38");
            const SFuncAnchor SetNextThink("f3 0f 10 44 24 34 8b 6c 24 30 0f 2e c1 8b 74 24 38", 0x7);
        }

        namespace CPortal_Player {
            const SFuncAnchor Spawn("8b 03 89 1c 24 ff 90 3c 08 00 00 83 c4 10 80 bb 10 0b 00 00 00", 0x62);

            const SFuncAnchor ClientCommand("55 89 e5 57 56 53 81 ec 9c 00 00 00 8b 7d 0c 8b 5d 08 8b 07 85 c0");

            const SFuncAnchor OnFullyConnected("8b 5c 24 4c 8b 10 50 ff 92 88 00 00 00 83 c4 10 84 c0", 0xc);

            const SFuncAnchor PlayerTransitionCompleteThink("8b 5c 24 28 8b 10 68 00 00 f8 3f 6a 00", 0xa);
        }

        namespace CProp_Portal {
            const SFuncAnchor Spawn("53 83 ec 14 8b 5c 24 1c 8b 03 53 ff 50 68 89 1c 24"); //Only used to find g_pMatchFramework 
        }

        namespace CEnvFade {
            const SFuncAnchor InputFade("89 c2 83 e2 01 29 d3 89 da 83 ca 04 a8 02 0f 45 da 89 da 83 ca 08 a8 08 0f 45 da a8 04", 0x16);
            const SFuncAnchor InputReverseFade("89 c6 83 e6 01 83 c6 01 89 f2 83 ca 04 a8 02 0f 45 f2 89 f2 83 ca 08 a8 08 0f 45 f2", 0x20);
        }

        //>>>>> matchmaking anchors <<<<<

        namespace CMatchTitleGameSettingsMgr {
            const SFuncAnchor InitializeGameSettings("57 56 53 8b 5c 24 14 8b 74 24 18 83 ec 04");
        }

        //>>>>> engine anchors <<<<<

        const SFuncAnchor SV_Frame("85 c0 74 14 89 f1 84 c9 74 0e 8b 10 83 ec 08 6a 01 50", 0x1f); //Only used to find the global sv variable

        namespace CGameServer {
            const SFuncAnchor InitMaxClients("8b 5c 24 20 c7 44 24 04 01 00 00 00 c7 44 24 08 40 00 00 00", 0xa);
        }
    }
}

#endif