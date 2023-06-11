#ifndef H_P2MPPATCH_PATCHES_UTILS
#define H_P2MPPATCH_PATCHES_UTILS

#include <stdexcept>
#include <sstream>
#include <iomanip>
#include "module.hpp"
#include "anchors.hpp"

namespace patches {
    inline void **get_server_global_g_pMatchFramework(const CModule& server_module) {
        //Check and decode a instruction accessing the global
        SAnchor CProp_Portal_Spawn = PATCH_FUNC_ANCHOR(server_module, CProp_Portal::Spawn);

        uint8_t *g_pMatchFramework_load_instr = (uint8_t*) CProp_Portal_Spawn.get_addr() + 0x4c;
        if(*g_pMatchFramework_load_instr != 0xa1) {
            std::ostringstream sstream;
            sstream << std::hex << std::setfill('0') << std::setw(2) << "CProp_Portal::Spawn global 'sv' access instruction (at " << g_pMatchFramework_load_instr << ") not as expected: " << *g_pMatchFramework_load_instr << " != a1\n";
            throw std::runtime_error(sstream.str());
        }

        void *ptr_g_pMatchFramework = *(void**) (g_pMatchFramework_load_instr + 1);
        if(ptr_g_pMatchFramework < server_module.base_addr() || (uint32_t*) server_module.base_addr() + server_module.size() <= ptr_g_pMatchFramework) {
            std::ostringstream sstream;
            sstream << "CProp_Portal::Spawn global 'sv' access instruction (at " << g_pMatchFramework_load_instr << ") references OOB address " << ptr_g_pMatchFramework << "\n";
            throw std::runtime_error(sstream.str());
        }

        DevMsg("&(g_pMatchFramework) = %p\n", ptr_g_pMatchFramework);
        return (void**) ptr_g_pMatchFramework;
    }

    inline void *get_engine_global_CBaseServer(const CModule& engine_module) {
        //Check and decode a instruction accessing the global
        SAnchor SV_Frame = PATCH_FUNC_ANCHOR(engine_module, SV_Frame);

        uint8_t *glob_sv_load_instr = (uint8_t*) SV_Frame.get_addr() + 0x4c;
        if(*glob_sv_load_instr != 0x68) {
            std::ostringstream sstream;
            sstream << std::hex << std::setfill('0') << std::setw(2) << "SV_Frame global 'sv' access instruction (at " << glob_sv_load_instr << ") not as expected: " << *glob_sv_load_instr << " != 68\n";
            throw std::runtime_error(sstream.str());
        }

        void *glob_sv = *(void**) (glob_sv_load_instr + 1);
        if(glob_sv < engine_module.base_addr() || (uint32_t*) engine_module.base_addr() + engine_module.size() <= glob_sv) {
            std::ostringstream sstream;
            sstream << "SV_Frame global 'sv' access instruction (at " << glob_sv_load_instr << ") references OOB address " << glob_sv << "\n";
            throw std::runtime_error(sstream.str());
        }

        DevMsg("&(sv) = %p\n", glob_sv);
        return glob_sv;
    }
}

#endif