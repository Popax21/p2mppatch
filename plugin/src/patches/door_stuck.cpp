#include <tier0/dbg.h>
#include <datamap.h>
#include <tier0/valve_minmax_off.h>

#include "plugin.hpp"
#include "anchors.hpp"
#include "door_stuck.hpp"
#include "utils.hpp"

using namespace patches;

int CDoorStuckPatch::OFF_CTriggerMultiple_m_flWait;
int CDoorStuckPatch::OFF_CTriggerMultiple_m_OnTrigger;
int CDoorStuckPatch::OFF_CBaseEntityOutput_m_ActionList;

CGlobalVars *CDoorStuckPatch::gpGlobals;
void **CDoorStuckPatch::ptr_g_pGameRules;

void CDoorStuckPatch::register_patches(CMPPatchPlugin& plugin) {
    //Hook CTriggerOnce::Spawn to reset m_flWait back to normal for door-related triggers
    OFF_CTriggerMultiple_m_flWait = anchors::server::CTriggerMultiple::m_flWait.get(plugin.server_module());
    OFF_CTriggerMultiple_m_OnTrigger = ((typedescription_t*) anchors::server::CTriggerMultiple::typedescription_m_OnTrigger.get(plugin.server_module()).get_addr())->fieldOffset[0];
    DevMsg("- offsetof(CTriggerMultiple::m_OnTrigger) = 0x%x\n", OFF_CTriggerMultiple_m_OnTrigger);
    OFF_CBaseEntityOutput_m_ActionList = anchors::server::CBaseEntityOutput::m_ActionList.get(plugin.server_module());

    gpGlobals = plugin.get_globals();
    ptr_g_pGameRules = (void**) anchors::server::g_pGameRules.get(plugin.server_module()).get_addr();

    CHookTracker& htrack_CTriggerOnce_Spawn = anchors::server::CTriggerOnce::Spawn.hook_tracker(plugin.server_module());
    plugin.register_patch<CFuncHook>(plugin.scratchpad(), htrack_CTriggerOnce_Spawn, (void*) &hook_CTriggerOnce_Spawn);
}

HOOK_FUNC void CDoorStuckPatch::hook_CTriggerOnce_Spawn(HOOK_ORIG int (*orig)(void*), void *trigger) {
    DevMsg("HOOK | CDoorStuckPatch | CTriggerOnce::Spawn | this=%p\n", trigger);
    orig(trigger);

    //Check if we should intervene
    if(!should_apply_sp_compat_patches(*ptr_g_pGameRules, gpGlobals)) return;

    //Check if this trigger is a door opening/closing trigger
    //The actual func_instance entities are skipped over in the final logic configuration, instead directly connecting to the func_io_proxy
    //The "cleaner" solution would be to detect this proxy, and check its outputs
    //But this also works .-.
    uint32_t *act_list = *(uint32_t**) ((uint8_t*) trigger + OFF_CTriggerMultiple_m_OnTrigger + OFF_CBaseEntityOutput_m_ActionList);

    DevMsg("CDoorStuckPatch | this->m_OnTrigger.m_ActionList = %p\n", act_list);
    if(!act_list) return;

    DevMsg("CDoorStuckPatch | m_ActionList->m_iTarget = '%s'\n", (char*) act_list[0]);
    DevMsg("CDoorStuckPatch | m_ActionList->m_iTargetInput = '%s'\n", (char*) act_list[1]);
    DevMsg("CDoorStuckPatch | m_ActionList->m_iParameter = '%s'\n", (char*) act_list[2]);
    if(!act_list[0] || !act_list[1] || strstr((char*) act_list[0], "door") == nullptr || strstr((char*) act_list[0], "proxy") == nullptr || strstr((char*) act_list[1], "OnProxyRelay") == nullptr) return;

    //Reset the wait time back to normal, so that the trigger returns to being a CTriggerMultiple
    assert(*(float*) ((uint8_t*) trigger + OFF_CTriggerMultiple_m_flWait) < 0);
    *(float*) ((uint8_t*) trigger + OFF_CTriggerMultiple_m_flWait) = 0.2f;

    //Also clear the "only once" flag of the output
    act_list[4] = (uint32_t) -1; //EVENT_FIRE_ALWAYS

    Msg("P2MPPatch | Transformed door opening/closing trigger_once %p into a trigger_multiple\n", trigger);
}