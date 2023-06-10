#ifndef H_P2MPPATCH_PATCHES_ANCHORS
#define H_P2MPPATCH_PATCHES_ANCHORS

#include <tier0/dbg.h>
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

        const SFuncAnchor GetNumPlayersConnected("8b 10 83 ec 0c 50 ff 92 58 01 00 00 83 c4 10 3c 01 83 de ff", 0x38);
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

        namespace CPortal_Player {
            const SFuncAnchor Spawn("8b 03 89 1c 24 ff 90 3c 08 00 00 83 c4 10 80 bb 10 0b 00 00 00", 0x62);

            const SFuncAnchor ClientCommand("55 89 e5 57 56 53 81 ec 9c 00 00 00 8b 7d 0c 8b 5d 08 8b 07 85 c0");

            const SFuncAnchor OnFullyConnected("8b 5c 24 4c 8b 10 50 ff 92 88 00 00 00 83 c4 10 84 c0", 0xc);

            const SFuncAnchor PlayerTransitionCompleteThink("8b 5c 24 28 8b 10 68 00 00 f8 3f 6a 00", 0xa);
        }

        //>>>>> matchmaking anchors <<<<<

        namespace CMatchTitleGameSettingsMgr {
            const SFuncAnchor InitializeGameSettings("57 56 53 8b 5c 24 14 8b 74 24 18 83 ec 04");
        }
    }
}

#endif