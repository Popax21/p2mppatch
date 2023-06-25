#include <stdexcept>
#include <chrono>
#include <system_error>
#include <tier0/dbg.h>
#include "module.hpp"

constexpr int PAGE_SIZE = CModule::PAGE_SIZE;

static bool find_module(const char *name, void **base_addr, size_t *size);

#ifdef LINUX
#include <link.h>
#include <sys/mman.h>

struct check_mod_params {
    const char *name;
    void *base_addr;
    size_t size;
    uint8_t *page_flags;
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
    size_t num_pages = (end_addr - start_addr) / PAGE_SIZE;
    params->page_flags = new uint8_t[num_pages];
    std::fill_n(params->page_flags, num_pages, 0);

    for(int i = 0; i < info->dlpi_phnum; i++) {
        Elf32_Word pflags = info->dlpi_phdr[i].p_flags;
        uint8_t flags = 0;
        if(pflags & PF_R) flags |= (uint8_t) CModule::EPageFlag::PAGE_R;
        if(pflags & PF_W) flags |= (uint8_t) CModule::EPageFlag::PAGE_W;
        if(pflags & PF_X) flags |= (uint8_t) CModule::EPageFlag::PAGE_X;

        uintptr_t ph_start = info->dlpi_addr + info->dlpi_phdr[i].p_vaddr, ph_end = ph_start + info->dlpi_phdr[i].p_memsz;
        for(uintptr_t i = ph_start / PAGE_SIZE; i*PAGE_SIZE < ph_end; i++) params->page_flags[i - start_addr / PAGE_SIZE] = flags;
    }

    params->base_addr = (void*) start_addr;
    params->size = (size_t) (end_addr - start_addr);

    DevMsg("Found module '%s' matching target name '%s' at addr %lx - %lx\n", info->dlpi_name, params->name, start_addr, end_addr);
    return 0;
}

static bool find_module(const char *name, void **base_addr, size_t *size, uint8_t **page_flags) {
    struct check_mod_params params;
    params.name = name;
    params.base_addr = nullptr;
    params.size = 0;
    params.page_flags = nullptr;

    dl_iterate_phdr(check_mod, &params);
    if(!params.base_addr) return false;

    *base_addr = params.base_addr;
    *size = params.size;
    *page_flags = params.page_flags;
    return true;
}

static void apply_page_flags(void *page, uint8_t flags) {
    int prot = 0;
    if(flags & (int) CModule::EPageFlag::PAGE_R) prot |= PROT_READ;
    if(flags & (int) CModule::EPageFlag::PAGE_W) prot |= PROT_WRITE;
    if(flags & (int) CModule::EPageFlag::PAGE_X) prot |= PROT_EXEC;
    if(mprotect(page, PAGE_SIZE, prot) < 0) {
        throw std::system_error(errno, std::system_category());
    }
}

#else
#error Implement me!
#endif

CModule::CModule(const char *name) : m_Name(name) {
    //Find the module
    if(!find_module(name, &m_BaseAddr, &m_Size, &m_PageFlags)) {
        std::stringstream sstream;
        sstream << "Can't find module '" << name << "'";
        throw std::runtime_error(sstream.str());
    }

    //Build suffix array
    using namespace std::chrono;
    auto t1 = high_resolution_clock::now();
    m_SufArray = new CSuffixArray(*this, MAX_NEEDLE_SIZE);
    auto t2 = high_resolution_clock::now();
    DevMsg("Constructed suffix array for module '%s' [%d kB], took %dms\n", name, m_SufArray->get_mem_usage() / 1024, (int) duration_cast<milliseconds>(t2 - t1).count());
}

CModule::~CModule() {
    reset_dependents();

    delete m_SufArray;
    m_SufArray = nullptr;
}

int CModule::compare(const IByteSequence &seq, size_t this_off, size_t seq_off, size_t size) const {
    while(size > 0) {
        size_t sz = std::min(size, PAGE_SIZE - (this_off % PAGE_SIZE));

        if(m_PageFlags[this_off / PAGE_SIZE] & (int) EPageFlag::PAGE_R) {
            int r = seq.compare((uint8_t*) m_BaseAddr + this_off, seq_off, sz);
            if(r != 0) return -r;
        } else {
            for(size_t o = 0; o < sz; o++) {
                if(seq[seq_off + o] != 0) return 1;
            }
        }

        this_off += sz;
        seq_off += sz;
        size -= sz;
    }

    return 0;
}

int CModule::compare(const uint8_t *buf, size_t off, size_t size) const {
    while(size > 0) {
        size_t sz = std::min(size, PAGE_SIZE - (off % PAGE_SIZE));

        if(m_PageFlags[off / PAGE_SIZE] & (int) EPageFlag::PAGE_R) {
            int r = memcmp((uint8_t*) m_BaseAddr + off, buf, sz);
            if(r != 0) return r;
        } else {
            for(const uint8_t *p = buf; sz > 0; p++, sz--) {
                if(*p != 0) return 1;
            }
        }

        buf += sz;
        off += sz;
        size -= sz;
    }

    return 0;
}

void CModule::get_data(uint8_t *buf, size_t off, size_t size) const {
    while(size > 0) {
        size_t sz = std::min(size, PAGE_SIZE - (off % PAGE_SIZE));

        if(m_PageFlags[off / PAGE_SIZE] & (int) EPageFlag::PAGE_R) memcpy((uint8_t*) m_BaseAddr + off, buf, sz);
        else memset(buf, 0, sz);

        buf += sz;
        off += sz;
        size -= sz;
    }
}

SAnchor CModule::find_seq_anchor(const IByteSequence &seq, IByteSequence *discrim_seq, ssize_t discrim_off) const {
    size_t off;

    using namespace std::chrono;
    auto t1 = high_resolution_clock::now();

    if(!m_SufArray->find_needle(seq, &off, discrim_seq, discrim_off)) {
        std::stringstream sstream;
        sstream << "Can't find sequence needle [" << seq.size() << " bytes] in module '" << m_Name << "'";
        throw std::runtime_error(sstream.str());
    }

    auto t2 = high_resolution_clock::now();

    DevMsg("Found sequence anchor for sequence '");
    for(size_t i = 0; i < seq.size(); i++) DevMsg("%02x", seq[i]);
    DevMsg("' [%d bytes] in module '%s' at offset 0x%zx, took %dms\n", seq.size(), m_Name, off, (int) duration_cast<milliseconds>(t2 - t1).count());

    return SAnchor(this, off); 
}

void CModule::unprotect() {
    size_t num_unprot = 0;
    for(size_t i = 0; i*PAGE_SIZE < m_Size; i++) {
        //Check if the page is readable, but isn't writeable
        if(!(m_PageFlags[i] & (int) EPageFlag::PAGE_R)) continue;
        if((m_PageFlags[i] & (int) EPageFlag::PAGE_W) || (m_PageFlags[i] & (int) EPageFlag::PAGE_UNPROT)) continue;

        //Make the page writeable
        apply_page_flags((uint8_t*) m_BaseAddr + i*PAGE_SIZE, m_PageFlags[i] | (uint8_t) EPageFlag::PAGE_W);
        m_PageFlags[i] |= (uint8_t) EPageFlag::PAGE_UNPROT;
        num_unprot++;
    }
    DevMsg("Unprotected 0x%zx pages in module '%s'\n", num_unprot, m_Name);
}

void CModule::reprotect() {
    size_t num_reprot = 0;
    for(size_t i = 0; i*PAGE_SIZE < m_Size; i++) {
        //Check if the page has been unprotected
        if(!(m_PageFlags[i] & (int) EPageFlag::PAGE_UNPROT)) continue;

        //Re-apply the old page flags 
        m_PageFlags[i] &= ~(uint8_t) EPageFlag::PAGE_UNPROT;
        apply_page_flags((uint8_t*) m_BaseAddr + i*PAGE_SIZE, m_PageFlags[i]);
        num_reprot++;
    }
    DevMsg("Reprotected 0x%zx pages in module '%s'\n", num_reprot, m_Name);
}

void CModule::associate_dependent(IModuleDependent *dep) {
    dep->associate(*this);
    m_Dependents.push_back(dep);
}

void CModule::reset_dependents() {
    for(IModuleDependent *dep : m_Dependents) dep->reset();
    if(m_Dependents.size() > 0) DevMsg("Reset %zd dependents of module '%s'\n", m_Dependents.size(), m_Name);
    m_Dependents.clear();
}