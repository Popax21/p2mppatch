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
    bool *page_flags;
};

static int check_mod(struct dl_phdr_info *info, size_t size, void *data) {
    struct check_mod_params *params = (struct check_mod_params*) data;

    //Check name
    int name_len = strlen(info->dlpi_name);
    if(name_len >= 3 && strcmp(info->dlpi_name + name_len-3, ".so") == 0) name_len -= 3;

    int targ_name_len = strlen(params->name);
    if(name_len < targ_name_len || strncmp(info->dlpi_name + name_len - targ_name_len, params->name, targ_name_len) != 0) return 0;
 
    //Find end address
    uintptr_t start_addr = info->dlpi_addr, end_addr = start_addr;
    for(int i = 0; i < info->dlpi_phnum; i++) {
        uintptr_t ph_end = info->dlpi_addr + info->dlpi_phdr[i].p_vaddr + info->dlpi_phdr[i].p_memsz;
        if(ph_end > end_addr) end_addr = ph_end;
    }

    //Align page flags
    start_addr -= (start_addr % PAGE_SIZE);
    if(end_addr % PAGE_SIZE) end_addr += PAGE_SIZE - (end_addr % PAGE_SIZE);

    //Create page flags
    params->page_flags = new bool[(end_addr - start_addr) / PAGE_SIZE];
    for(int i = 0; i < info->dlpi_phnum; i++) {
        //Skip writeable segments
        if(info->dlpi_phdr[i].p_flags & PF_W) continue;

        //Set flags
        uintptr_t ph_start = info->dlpi_addr + info->dlpi_phdr[i].p_vaddr, ph_end = ph_start + info->dlpi_phdr[i].p_memsz;
        for(uintptr_t i = ph_start / PAGE_SIZE; i*PAGE_SIZE < ph_end; i++) params->page_flags[i - start_addr / PAGE_SIZE] = true;
    }

    params->base_addr = (void*) start_addr;
    params->size = (size_t) (end_addr - start_addr);

    DevMsg("Found module '%s' matching target name '%s' at addr %lx - %lx\n", info->dlpi_name, params->name, start_addr, end_addr);
    return 0;
}

static bool find_module(const char *name, void **base_addr, size_t *size, bool **page_flags) {
    struct check_mod_params params;
    params.name = name;
    params.base_addr = NULL;
    params.size = 0;
    params.page_flags = NULL;

    dl_iterate_phdr(check_mod, &params);
    if(!params.base_addr) return false;

    *base_addr = params.base_addr;
    *size = params.size;
    *page_flags = params.page_flags;
    return true;
}
#else
#error Implement me!
#endif

CModule::CModule(const char *name) {
    //Find the module
    if(!find_module(name, &m_BaseAddr, &m_Size, &m_PageFlags)) throw std::runtime_error("Can't find module '" + std::string(name) + "'!");

    //Build suffix tree
    m_SufTree = new CSuffixTree(*this);
    DevMsg("Constructed suffix tree for module '%s' [%d kB]\n", name, m_SufTree->get_mem_usage() / 1024);
}

CModule::~CModule() {
    delete m_SufTree;
    m_SufTree = nullptr;
}

bool CModule::compare(const IByteSequence &seq, size_t this_off, size_t seq_off, size_t size) const {
    while(size > 0) {
        size_t sz = std::min(size, PAGE_SIZE - (this_off % PAGE_SIZE));

        if(m_PageFlags[this_off / PAGE_SIZE]) {
            if(!seq.compare((uint8_t*) m_BaseAddr + this_off, seq_off, sz)) return false;
        }

        this_off += sz;
        seq_off += sz;
        size -= sz;
    }

    return true;
}

bool CModule::compare(const uint8_t *buf, size_t off, size_t size) const {
    while(size > 0) {
        size_t sz = std::min(size, PAGE_SIZE - (off % PAGE_SIZE));

        if(m_PageFlags[off / PAGE_SIZE]) {
            if(!memcmp((uint8_t*) m_BaseAddr + off, buf, sz)) return false;
        }

        buf += sz;
        off += sz;
        size -= sz;
    }

    return true;
}

void CModule::get_data(uint8_t *buf, size_t off, size_t size) const {
    while(size > 0) {
        size_t sz = std::min(size, PAGE_SIZE - (off % PAGE_SIZE));

        if(m_PageFlags[off / PAGE_SIZE]) memcpy((uint8_t*) m_BaseAddr + off, buf, sz);
        else memset(buf, 0, sz);

        buf += sz;
        off += sz;
        size -= sz;
    }
}