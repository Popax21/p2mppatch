#ifndef H_P2MPPATCH_MODULE
#define H_P2MPPATCH_MODULE

#include "byteseq.hpp"
#include "suftree.hpp"
#include "anchor.hpp"

#define PAGE_SIZE 0x1000

class CModule : public IByteSequence {
    public:
        enum class page_flag {
            PAGE_R = 1, PAGE_W = 2, PAGE_X = 4, PAGE_UNPROT = 8
        };

        CModule(const char *name);
        ~CModule();

        virtual const char *name() const { return m_Name; }
        virtual void *base_addr() const { return m_BaseAddr; }
        virtual size_t size() const { return m_Size; };

        virtual bool compare(const IByteSequence &seq, size_t this_off, size_t seq_off, size_t size) const;
        virtual bool compare(const uint8_t *buf, size_t off, size_t size) const;
        virtual void get_data(uint8_t *buf, size_t off, size_t size) const;

        virtual SAnchor find_seq_anchor(const IByteSequence &seq) const;

        virtual void unprotect();
        virtual void reprotect();

        virtual uint8_t operator [](size_t off) const {
            if((m_PageFlags[off / PAGE_SIZE] & (int) page_flag::PAGE_R) == 0) return 0;
            return ((uint8_t*) m_BaseAddr)[off];
        }

    private:
        const char *m_Name;
        void *m_BaseAddr;
        size_t m_Size;

        uint8_t *m_PageFlags;
        CSuffixTree *m_SufTree;
};

#endif