#ifndef H_P2MPPATCH_PATCHES_UTILS
#define H_P2MPPATCH_PATCHES_UTILS

#include <edict.h>
#include <tier0/valve_minmax_off.h>

#include <string.h>
#include "vtab.hpp"

namespace patches {
    inline bool should_apply_sp_compat_patches(void *game_rules, CGlobalVars *globals) {
        return PATCH_VTAB_FUNC(game_rules, server::CGameRules::IsMultiplayer)(game_rules) && strncmp(globals->mapname.ToCStr(), "sp_", 3) == 0;
    }
}

#endif