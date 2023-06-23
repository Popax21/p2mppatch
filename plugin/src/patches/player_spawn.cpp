#include <tier0/dbg.h>
#include <engine/IEngineTrace.h>
#include <tier0/valve_minmax_off.h>

#include <string.h>
#include "plugin.hpp"
#include "anchors.hpp"
#include "vtab.hpp"
#include "player_spawn.hpp"
#include "transitions_fix.hpp"
#include "utils.hpp"

using namespace patches;

int CPlayerSpawnPatch::OFF_CBaseEntity_m_CollisionGroup;

void *CPlayerSpawnPatch::engine_trace;
CGlobalVars *CPlayerSpawnPatch::gpGlobals;
void **CPlayerSpawnPatch::ptr_g_pGameRules;
void **CPlayerSpawnPatch::ptr_g_pLastSpawn;

void *(*CPlayerSpawnPatch::UTIL_PlayerByIndex)(int idx);
void (*CPlayerSpawnPatch::CPointTeleport_DoTeleport)(void *teleport, void *inputdata, Vector *vecOrigin, QAngle *angRotation, uintptr_t bOverrideTarget);

void CPlayerSpawnPatch::register_patches(CMPPatchPlugin& plugin) {
    SAnchor CPointTeleport_DoTeleport = anchors::server::CPointTeleport::DoTeleport.get(plugin.server_module());

    //Obtain needed misc information for the detour
    OFF_CBaseEntity_m_CollisionGroup = anchors::server::CBaseEntity::m_CollisionGroup.get(plugin.server_module());

    engine_trace = plugin.engine_trace();
    gpGlobals = plugin.get_globals();
    ptr_g_pGameRules = (void**) anchors::server::g_pGameRules.get(plugin.server_module()).get_addr();
    ptr_g_pLastSpawn = (void**) anchors::server::g_pLastSpawn.get(plugin.server_module()).get_addr();

    UTIL_PlayerByIndex = (void *(*)(int)) anchors::server::UTIL_PlayerByIndex.get(plugin.server_module()).get_addr();
    CPlayerSpawnPatch::CPointTeleport_DoTeleport = (void (*)(void*, void*, Vector*, QAngle*, uintptr_t)) CPointTeleport_DoTeleport.get_addr();

    //Patch CPointTeleport::DoTeleport to allow for passing in an entity pointer directly by giving a null inputdata pointer
    verify_anchor_sequence(CPointTeleport_DoTeleport + 0x4f, "89 c3 85 c0 0f 84 ?? ?? ?? ??");
    plugin.register_patch<CSeqPatch>(CPointTeleport_DoTeleport + 0x1c, new SEQ_HEX("8b 50 04 8b 18"), new SEQ_SCRATCH_DETOUR(plugin, 5, new SEQ_SEQ(
        new SEQ_HEX("66 9c 85 c0 75 0b 66 9d 8b 44 24 40"), //pushf; test eax, eax; jnz <orig code>; popf; mov eax, dword ptr [esp + 0x40 <bOverrideTarget>] 
        new SEQ_JMP(CPointTeleport_DoTeleport + 0x4f),
        new SEQ_HEX("66 9d 8b 50 04 8b 18"), //popf; <orig code>
    )));

    //Detour the gEntList.FindEntityByName call in CPointTeleport::DoTeleport
    plugin.register_patch<CSeqPatch>(CPointTeleport_DoTeleport + 0x37, new SEQ_MASKED_HEX("83 ec 04 6a 00"),
        (new SEQ_FUNC_DETOUR(plugin, 5, detour_CPointTeleport_DoTeleport, DETOUR_ARG_ESI, DETOUR_ARG_EAX, DETOUR_ARG_EDI, DETOUR_ARG_EBP))->append_orig_suffix()
    );
}

class CSpawnGroundTraceFilter : public CTraceFilter {
    public:
        CSpawnGroundTraceFilter(int off) : OFF_CBaseEntity_m_CollisionGroup(off) {}

	    virtual bool ShouldHitEntity(IHandleEntity *pEntity, int contentsMask) override {
            void *ent = ((IServerUnknown*) pEntity)->GetBaseEntity();
            int col_group = *(int*) ((uint8_t*) ent + OFF_CBaseEntity_m_CollisionGroup);
            return col_group == COLLISION_GROUP_NONE; //Only collide with "world entities" (so e.g. no players, clutter, decorations, ...)
        }

    private:
        int OFF_CBaseEntity_m_CollisionGroup;
};

DETOUR_FUNC void CPlayerSpawnPatch::detour_CPointTeleport_DoTeleport(void **ptr_teleport, const char **ptr_entName, Vector **ptr_vecOrigin, QAngle **ptr_angRotation) {
    void *teleport = *ptr_teleport;
    Vector *vecOrigin = *ptr_vecOrigin;
    QAngle *angRotation = *ptr_angRotation;
    DevMsg("DETOUR | CPlayerSpawnPatch | CPointTeleport::DoTeleport | this=%p entName='%s' vecOrigin=%p angRotation=%p\n", teleport, *ptr_entName, vecOrigin, angRotation);

    //Check if we should be intervening
    if(!should_apply_sp_compat_patches(*ptr_g_pGameRules, gpGlobals)) return;

    //Check if we are targeting "the player"
    if(strcmp(*ptr_entName, "!player") != 0) return;
    *ptr_entName = nullptr;

    //Update the spawnpoint
    void *spawn = *ptr_g_pLastSpawn;
    if(spawn) {
        //Teleport the spawnpoint
        PATCH_VTAB_FUNC(spawn, server::CBaseEntity::SetParent)(spawn, nullptr, -1);
        CPointTeleport_DoTeleport(teleport, nullptr, vecOrigin, angRotation, (uintptr_t) spawn);

        //Shoot a raycast down to find the ground entity
        Ray_t ray;
        trace_t trace;
        CSpawnGroundTraceFilter filter(OFF_CBaseEntity_m_CollisionGroup);
        ray.Init(*vecOrigin, Vector(vecOrigin->x, vecOrigin->y - 8192, vecOrigin->z));
        PATCH_VTAB_FUNC(engine_trace, engine::IEngineTrace::TraceRay)(engine_trace, ray, MASK_PLAYERSOLID, &filter, &trace);
        DevMsg("CPlayerSpawnPatch | spawnpoint ground TraceRay: DidHit=%d fraction=%f m_pEnt=%p allsolid=%d startsolid=%d\n", trace.DidHit(), trace.fraction, trace.m_pEnt, trace.allsolid, trace.startsolid);

        //Anchor the the ground entity
        void *ground_ent = trace.DidHit() ? trace.m_pEnt : nullptr;
        PATCH_VTAB_FUNC(spawn, server::CBaseEntity::SetParent)(spawn, ground_ent, -1);

        if(ground_ent) Msg("P2MPPatch | Updated spawnpoint after CPointTeleport %p '!player' teleport, parented to ground entity %p\n", teleport, ground_ent);
        else Msg("P2MPPatch | Updated spawnpoint after CPointTeleport %p '!player' teleport, not parented to any ground entity\n", teleport);
    }

    //Teleport all players
    if(CTransitionsFixPatch::is_everyone_ready(*ptr_g_pGameRules)) {
        int num_tped_players = 0;
        for(int i = 1; i <= gpGlobals->maxClients; i++) {
            void *player = UTIL_PlayerByIndex(i);
            if(!player) continue;

            CPointTeleport_DoTeleport(teleport, nullptr, vecOrigin, angRotation, (uintptr_t) player);
            num_tped_players++;
        }

        Msg("P2MPPatch | Teleported %d players for CPointTeleport %p with target '!player'\n", num_tped_players, teleport);
    } else {
        //Don't teleport players as not everyone is ready yet
        Msg("P2MPPatch | Skipped teleporting players for CPointTeleport %p targeting '!player' as not everyone is ready yet\n", teleport);
    }
}