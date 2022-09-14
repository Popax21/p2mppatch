#include <stdexcept>
#include <tier0/dbg.h>
#include "module.hpp"

static bool find_module(const char *name, void **base_addr, size_t *size);

#ifdef LINUX
#include <link.h>

struct check_mod_params {
    const char *name;
    void *base_addr;
    size_t size;
};

static int check_mod(struct dl_phdr_info *info, size_t size, void *data) {
    struct check_mod_params *params = (struct check_mod_params*) data;

    //Check name
    int name_len = strlen(info->dlpi_name);
    if(name_len >= 3 && strcmp(info->dlpi_name + name_len-3, ".so") == 0) name_len -= 3;

    int targ_name_len = strlen(params->name);
    if(name_len < targ_name_len || strncmp(info->dlpi_name + name_len - targ_name_len, params->name, targ_name_len) != 0) return 0;
 
    //Find code / RO data segment
    uintptr_t start_addr = 0, end_addr = 0;
    for(int i = 0; i < info->dlpi_phnum; i++) {
        //Check if the segment is writeable
        if(info->dlpi_phdr[i].p_flags & PF_W) continue;

        uintptr_t ph_start = info->dlpi_addr + info->dlpi_phdr[i].p_vaddr, ph_end = ph_start + info->dlpi_phdr[i].p_memsz;
        if(end_addr < ph_end) end_addr = ph_end;

        if(start_addr == end_addr) {
            //Check if the segments are contiguous
            if(start_addr == ph_end) start_addr = ph_start;
            else if(end_addr == ph_start) end_addr = ph_end;
            else Warning("Skipping module '%s' code / RO data segment %lx - %lx not contiguous with %lx-%lx\n", info->dlpi_name, ph_start, ph_end, start_addr, end_addr);
        } else {
            start_addr = ph_start;
            end_addr = ph_end;
        }
    }

    if(start_addr == end_addr) {
        Warning("Found matching module '%s' without code segment! Skipping...\n", info->dlpi_name);
        return 0;
    }

    params->base_addr = (void*) start_addr;
    params->size = (size_t) (end_addr - start_addr);

    DevMsg("Found module '%s' matching target name '%s' at addr %lx: code/RO data %lx - %lx\n", info->dlpi_name, params->name, info->dlpi_addr, start_addr, end_addr);
    return 0;
}

static bool find_module(const char *name, void **base_addr, size_t *size) {
    struct check_mod_params params;
    params.name = name;
    params.base_addr = NULL;
    params.size = 0;

    dl_iterate_phdr(check_mod, &params);

    if(!params.base_addr) return false;
    *base_addr = params.base_addr;
    *size = params.size;
    return true;
}
#else
#error Implement me!
#endif

CModule::CModule(const char *name) {
    //Find the module
    if(!find_module(name, &m_BaseAddr, &m_Size)) throw std::runtime_error("Can't find module '" + std::string(name) + "'!");

    //Align module to page boundaries
    m_Size += ((uintptr_t) m_BaseAddr) % PAGE_SIZE;
    m_BaseAddr = (void*) (((uintptr_t) m_BaseAddr) - ((uintptr_t) m_BaseAddr) % PAGE_SIZE);
    if(m_Size % PAGE_SIZE) m_Size += PAGE_SIZE - (m_Size % PAGE_SIZE);

    //Build suffix tree
    m_SufTree = new CSuffixTree(*this);
    DevMsg("Constructed suffix tree for module '%s' [%d kB]\n", name, m_SufTree->get_mem_usage() / 1024);
}

CModule::~CModule() {
    delete m_SufTree;
    m_SufTree = nullptr;
}