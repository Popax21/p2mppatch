#ifndef H_P2MPPATCH_SCRATCHPAD
#define H_P2MPPATCH_SCRATCHPAD

#include "anchor.hpp"
#include "byteseq.hpp"

class CScratchPad {
    public:
        static const int MAX_SCRATCH_PAGES = 1024;
        static const int SCRATCH_PAGE_SIZE = 8192;

        struct SSeqEntry {
            public:
                SSeqEntry() : m_ScratchPad(nullptr) {}
                SSeqEntry(SSeqEntry& o) : m_ScratchPad(o.m_ScratchPad), m_PageIdx(o.m_PageIdx), m_Anchor(o.m_Anchor), m_SeqSize(o.m_SeqSize) { o.m_ScratchPad = nullptr; }
                SSeqEntry(const SSeqEntry& o) = delete;
                ~SSeqEntry() { if(m_ScratchPad) m_ScratchPad->free_entry(*this); }

                SAnchor anchor() const { return m_Anchor; }
                size_t seq_size() const { return m_SeqSize; }

                SSeqEntry& operator =(SSeqEntry&& v) {
                    if(m_ScratchPad) m_ScratchPad->free_entry(*this);
                    m_ScratchPad = v.m_ScratchPad;
                    m_PageIdx = v.m_PageIdx;
                    m_Anchor = v.m_Anchor;
                    m_SeqSize = v.m_SeqSize;
                    v.m_ScratchPad = nullptr;
                    return *this;
                }

            private:
                SSeqEntry(CScratchPad *scratch, int page_idx, SAnchor anchor, size_t seq_size) : m_ScratchPad(scratch), m_PageIdx(page_idx), m_Anchor(anchor), m_SeqSize(seq_size) {}

                CScratchPad *m_ScratchPad;
                int m_PageIdx;
                SAnchor m_Anchor;
                size_t m_SeqSize;

                friend class CScratchPad;
        };

        CScratchPad() : m_TotalSize(0) {
            std::fill_n(m_Pages, MAX_SCRATCH_PAGES, nullptr);
            std::fill_n(m_CurPageOff, MAX_SCRATCH_PAGES, 0);
            std::fill_n(m_PageRefCnts, MAX_SCRATCH_PAGES, 0);
        }
        CScratchPad(CScratchPad& o) = delete;
        CScratchPad(const CScratchPad& o) = delete;
        ~CScratchPad() { clear(); }

        SSeqEntry alloc_seq(IByteSequence& seq);
        void clear();

        size_t total_size() const { return m_TotalSize; }

    protected:
        size_t m_TotalSize;
        void *m_Pages[MAX_SCRATCH_PAGES];
        size_t m_CurPageOff[MAX_SCRATCH_PAGES];
        int m_PageRefCnts[MAX_SCRATCH_PAGES];

        void free_entry(const SSeqEntry& entry);
};

#endif