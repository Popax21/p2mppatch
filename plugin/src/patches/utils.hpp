#ifndef H_P2MPPATCH_PATCHES_UTILS
#define H_P2MPPATCH_PATCHES_UTILS

#include <edict.h>
#include <tier0/valve_minmax_off.h>

#include <string.h>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include "byteseq.hpp"
#include "vtab.hpp"

namespace patches {
    inline void verify_anchor_sequence(SAnchor anchor, const char *hex_seq) {
        CMaskedHexSequence seq(hex_seq);
        if(seq.compare((const uint8_t*) anchor.get_addr(), 0, seq.size()) == 0) return;

        std::stringstream sstream;
        sstream << "Unexpected sequence at anchor " << anchor.debug_str() << ": " << std::hex << std::setw(2) << std::setfill('0');
        for(int i = 0; i < seq.size(); i++) sstream << ((const uint8_t*) anchor.get_addr())[i];
        sstream << " != ";
        for(int i = 0; i < seq.size(); i++) sstream << seq[i];
        throw std::runtime_error(sstream.str());
    }

    inline bool should_apply_sp_compat_patches(void *game_rules, CGlobalVars *globals) {
        return PATCH_VTAB_FUNC(game_rules, server::CGameRules::IsMultiplayer)(game_rules) && strncmp(globals->mapname.ToCStr(), "sp_", 3) == 0;
    }
}

#endif