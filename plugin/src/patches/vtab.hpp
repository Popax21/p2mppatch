#ifndef H_P2MPPATCH_PATCHES_VTAB
#define H_P2MPPATCH_PATCHES_VTAB

#include <stdint.h>

struct Ray_t;
class CGameTrace;
typedef CGameTrace trace_t;
class ITraceFilter;

namespace patches::vtabs {
    #define PATCH_VTAB_FUNC(obj, func) ((patches::vtabs::func##_FTYPE) (*(void***) (obj))[patches::vtabs::func##_IDX])

    #define PATCH_DECL_VTAB_FUNC(name, idx, ret_type) \
        const int name##_IDX = idx; \
        typedef ret_type (*name##_FTYPE)(void *self);

    #define PATCH_DECL_VTAB_FUNC_ARGS(name, idx, ret_type, ...) \
        const int name##_IDX = idx; \
        typedef ret_type (*name##_FTYPE)(void *self, __VA_ARGS__);

    //>>>>> server vtables <<<<<

    namespace server {
        namespace CGameRules {
            PATCH_DECL_VTAB_FUNC(IsMultiplayer, 34, bool);
        }

        namespace CBaseEntity {
            PATCH_DECL_VTAB_FUNC_ARGS(SetParent, 39, void, void *pNewParent, int iAttachment);
            PATCH_DECL_VTAB_FUNC(IsPlayer, 86, bool);
        }
    }

    //>>>>> matchmaking vtables <<<<<

    namespace matchmaking {
        namespace IMatchFramework {
            PATCH_DECL_VTAB_FUNC(GetMatchSession, 12, void*);
            PATCH_DECL_VTAB_FUNC(GetMatchNetworkMsgController, 13, void*);
        }

        namespace IMatchSession {
            PATCH_DECL_VTAB_FUNC(GetSessionSystemData, 0, void*);
            PATCH_DECL_VTAB_FUNC(GetSessionSettings, 1, void*);
            PATCH_DECL_VTAB_FUNC_ARGS(UpdateSessionSettings, 2, void, void *pSettings);
            PATCH_DECL_VTAB_FUNC_ARGS(Command, 3, void, void *pCommand);
            PATCH_DECL_VTAB_FUNC(GetSessionID, 4, uint64_t);
            PATCH_DECL_VTAB_FUNC_ARGS(UpdateTeamProperties, 5, void, void *pTeamProperties);
        }

        namespace IMatchNetworkMsgController {
            PATCH_DECL_VTAB_FUNC_ARGS(GetActiveServerGameDetails, 1, void*, void *pRequest);
            PATCH_DECL_VTAB_FUNC_ARGS(UnpackGameDetailsFromSteamLobby, 3, void*, uint64_t uiLobbyI);
            PATCH_DECL_VTAB_FUNC_ARGS(PackageGameDetailsForReservation, 5, void*, void *pSettings);
        }
    }

    //>>>>> engine vtables <<<<<

    namespace engine {
        //This interface mismatches the one our SDK ships with ._:
        namespace IEngineTrace {
            PATCH_DECL_VTAB_FUNC_ARGS(TraceRay, 5, void, const Ray_t &ray, unsigned int fMask, ITraceFilter *pTraceFilter, trace_t *pTrace);
        }
    }
}

#endif