#ifndef H_P2MPPATCH_PATCHES_VTAB
#define H_P2MPPATCH_PATCHES_VTAB

#include <stdint.h>

class KeyValues;

namespace patches {
    namespace vtabs {
        #define PATCH_VTAB_FUNC(obj, func) ((patches::vtabs::func##_FTYPE) (*(void***) (obj))[patches::vtabs::func##_IDX])

        #define PATCH_DECL_VTAB_FUNC(name, idx, ret_type) \
            const int name##_IDX = idx; \
            typedef ret_type (*name##_FTYPE)(void *self);

        #define PATCH_DECL_VTAB_FUNC_ARGS(name, idx, ret_type, args) \
            const int name##_IDX = idx; \
            typedef ret_type (*name##_FTYPE)(void *self, args);

        //>>>>> matchmaking vtables <<<<<

        namespace IMatchFramework {
            PATCH_DECL_VTAB_FUNC(GetMatchSession, 12, void*);
            PATCH_DECL_VTAB_FUNC(GetMatchNetworkMsgController, 13, void*);
        }

        namespace IMatchSession {
            PATCH_DECL_VTAB_FUNC(GetSessionSystemData, 0, KeyValues*);
            PATCH_DECL_VTAB_FUNC(GetSessionSettings, 1, KeyValues*);
            PATCH_DECL_VTAB_FUNC_ARGS(UpdateSessionSettings, 2, void, KeyValues *pSettings);
            PATCH_DECL_VTAB_FUNC_ARGS(Command, 3, void, KeyValues *pCommand);
            PATCH_DECL_VTAB_FUNC(GetSessionID, 4, uint64_t);
            PATCH_DECL_VTAB_FUNC_ARGS(UpdateTeamProperties, 5, void, KeyValues *pTeamProperties);
        }

        namespace IMatchNetworkMsgController {
            PATCH_DECL_VTAB_FUNC_ARGS(GetActiveServerGameDetails, 1, KeyValues*, KeyValues *pRequest);
            PATCH_DECL_VTAB_FUNC_ARGS(UnpackGameDetailsFromSteamLobby, 3, KeyValues*, uint64_t uiLobbyI);
            PATCH_DECL_VTAB_FUNC_ARGS(PackageGameDetailsForReservation, 5, KeyValues*, KeyValues *pSettings);
        }
    }
}

#endif