#include <tier0/dbg.h>
#include <datamap.h>
#include <tier0/valve_minmax_off.h>

#include "plugin.hpp"
#include "anchors.hpp"
#include "env_fade.hpp"
#include "utils.hpp"

#define SF_FADE_ONLYONE 0x0004

using namespace patches;

CGlobalVars *CEnvFadePatch::gpGlobals;
void **CEnvFadePatch::ptr_g_pGameRules;

void CEnvFadePatch::register_patches(CMPPatchPlugin& plugin) {
    //Patch CEnvFade to never apply a fade to all clients (setting SF_FADE_ONLYONE)
    gpGlobals = plugin.get_globals();
    ptr_g_pGameRules = (void**) anchors::server::g_pGameRules.get(plugin.server_module()).get_addr();

    SAnchor CEnvFade_InputFade = anchors::server::CEnvFade::InputFade.get(plugin.server_module());
    plugin.register_patch<CSeqPatch>(CEnvFade_InputFade + 0x2c, new SEQ_HEX("18 08 0f 45 da"),
        (new SEQ_FUNC_DETOUR(plugin, 5, DETOUR_CEnvFade_InputFade_InputReverseFade, DETOUR_ARG_EDI, DETOUR_ARG_EAX))->prepend_orig_prefix()
    );

    SAnchor CEnvFade_InputReverseFade = anchors::server::CEnvFade::InputReverseFade.get(plugin.server_module());
    plugin.register_patch<CSeqPatch>(CEnvFade_InputReverseFade + 0x37, new SEQ_HEX("a8 08 0f 45 f2"),
        (new SEQ_FUNC_DETOUR(plugin, 5, DETOUR_CEnvFade_InputFade_InputReverseFade, DETOUR_ARG_EDI, DETOUR_ARG_EAX))->prepend_orig_prefix()
    );
}

DETOUR_FUNC void CEnvFadePatch::DETOUR_CEnvFade_InputFade_InputReverseFade(void ***ptr_inputdata, uint8_t *ptr_m_spawnflags) {
    void **inputdata = *ptr_inputdata;
    DevMsg("DETOUR | CEnvFadePatch | CEnvFade::Input[Reverse]Fade | inputdata=%p m_spawnflags=%x\n", inputdata, *ptr_m_spawnflags);

    //Check if we should intervene
    if(*ptr_m_spawnflags & SF_FADE_ONLYONE || !should_apply_sp_compat_patches(*ptr_g_pGameRules, gpGlobals)) return;

    //Check if there is an activator (the older engine lacks a null check here)
    if(!*inputdata) {
        Warning("P2MPPatch | Not intervening with non-SF_FADE_ONLYONE env_fade Input[Reverse]Fade signal as there is no activator entity\n");
        return;
    }

    *ptr_m_spawnflags |= SF_FADE_ONLYONE;
    Msg("P2MPPatch | Limited non-SF_FADE_ONLYONE env_fade Input[Reverse]Fade to only affecting activator entity %p\n", *inputdata);
}