#include <stdexcept>
#include <system_error>
#include <tier0/dbg.h>
#include "scratchpad.hpp"

#ifdef LINUX
#include <sys/mman.h>

static inline void *alloc_page() {
    void *page = mmap(NULL, CScratchPad::SCRATCH_PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(page == MAP_FAILED) throw std::system_error(errno, std::system_category());
    return page;
}

static inline void free_page(void *page) {
    if(munmap(page, CScratchPad::SCRATCH_PAGE_SIZE) < 0) throw std::system_error(errno, std::system_category());
}

#else
#error Implement me!
#endif

CScratchPad::SSeqEntry CScratchPad::alloc_seq(IByteSequence& seq) {
    size_t seq_size = seq.size();
    int page_idx;

    //Check if any existing pages have enough space left
    for(int i = 0; i < MAX_SCRATCH_PAGES; i++) {
        if(!m_Pages[i]) continue;
        if(SCRATCH_PAGE_SIZE - m_CurPageOff[i] >= seq_size) {
            page_idx = i;
            goto found_page;
        }
    }

    //Try to allocate a new page
    for(int i = 0; i < MAX_SCRATCH_PAGES; i++) {
        if(!m_Pages[i]) {
            m_Pages[i] = alloc_page();
            m_CurPageOff[i] = 0;
            m_PageRefCnts[i] = 0;
            page_idx = i;

            DevMsg("Allocated scratchpad page %d at %p (%d bytes)\n", i, m_Pages[i], CScratchPad::SCRATCH_PAGE_SIZE);
            goto found_page;
        }
    }

    throw std::runtime_error("Insufficient scratchpad memory available!");

    found_page:;

    //Store the sequence in the scratchpad page
    SAnchor seq_anchor = SAnchor(m_Pages[page_idx] + m_CurPageOff[page_idx]);
    m_CurPageOff[page_idx] += seq_size;
    m_PageRefCnts[page_idx]++;
    m_TotalSize += seq_size;

    SSeqEntry seq_entry(this, page_idx, seq_anchor, seq_size);

    if(!seq.apply_anchor(seq_anchor)) throw std::runtime_error("Couldn't anchor scratchpad sequence!");
    seq.get_data((uint8_t*) seq_anchor.get_addr(), 0, seq_size);

    DevMsg("Allocated %d byte scratchpad entry in page %d [%p]\n", seq_size, page_idx, seq_anchor.get_addr());

    return seq_entry;
}

void CScratchPad::clear() {
    //Free all pages
    for(int i = 0; i < MAX_SCRATCH_PAGES; i++) {
        if(m_Pages[i]) {
            if(m_PageRefCnts[i] > 0) Warning("Deallocating scratchpad page %d [%p] while there are still %d remaining references!\n", i, m_Pages[i], m_PageRefCnts[i]);
            free_page(m_Pages[i]);
        }

        m_Pages[i] = nullptr;
        m_CurPageOff[i] = 0;
        m_PageRefCnts[i] = 0;
    }

    m_TotalSize = 0;
}

void CScratchPad::free_entry(const SSeqEntry& entry) {
    DevMsg("Freeing %d byte scratchpad entry in page %d [%p]\n", seq_size, page_idx, seq_anchor.get_addr());

    m_TotalSize -= entry.m_SeqSize;
    if(--m_PageRefCnts[entry.m_PageIdx] <= 0) {
        //Free the page
        void *page = m_Pages[entry.m_PageIdx];
        free_page(page);
        m_Pages[entry.m_PageIdx] = nullptr;
        m_CurPageOff[entry.m_PageIdx] = 0;
        m_PageRefCnts[entry.m_PageIdx] = 0;

        DevMsg("Freeed scratchpad page %d at %p as it is no longer in use\n", entry.m_PageIdx, page);
    }
}