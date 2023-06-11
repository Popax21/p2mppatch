#ifndef H_P2MPPATCH_PATCHES_UTILS
#define H_P2MPPATCH_PATCHES_UTILS

#include <stdexcept>
#include <sstream>
#include <iomanip>
#include "module.hpp"
#include "anchors.hpp"

namespace patches {
    inline void *get_engine_global_CBaseServer(const CModule& engine_module) {
        //Check and decode a instruction accessing the global
        SAnchor SV_Frame = PATCH_FUNC_ANCHOR(engine_module, SV_Frame);

        uint8_t *glob_sv_load_instr = (uint8_t*) SV_Frame.get_addr() + 0x4c;
        if(*glob_sv_load_instr != 0x68) {
            std::ostringstream sstream;
            sstream << std::hex << std::setfill('0') << std::setw(2) << "CBaseServer::CheckTimeouts global 'sv' access instruction (at " << glob_sv_load_instr << ") not as expected: " << *glob_sv_load_instr << " != 68\n";
            throw std::runtime_error(sstream.str());
        }

        void *glob_sv = *(void**) (glob_sv_load_instr + 1);
        if(glob_sv < engine_module.base_addr() || (uint32_t*) engine_module.base_addr() + engine_module.size() <= glob_sv) {
            std::ostringstream sstream;
            sstream << "CBaseServer::CheckTimeouts global 'sv' access instruction (at " << glob_sv_load_instr << ") references OOB address " << glob_sv << "\n";
            throw std::runtime_error(sstream.str());
        }

        DevMsg("&(sv) = %p\n", glob_sv);
        return glob_sv;
    }
}

#endif