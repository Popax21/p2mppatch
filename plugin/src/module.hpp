#ifndef H_P2MPPATCH_MODULE
#define H_P2MPPATCH_MODULE

#include "byteseq.hpp"
#include "suftree.hpp"

#define PAGE_SIZE 0x1000

class CModule : public IByteSequence {
    public:
        CModule(const char *name);
        ~CModule();

        virtual size_t size() const { return m_Size; };
        virtual const uint8_t *buffer() const { return (uint8_t*) m_BaseAddr; };
        virtual void get_data(uint8_t *buf, size_t off, size_t size) const { memcpy(buf, (uint8_t*) m_BaseAddr + off, size); }
        virtual uint8_t operator [](size_t off) const { return ((uint8_t*) m_BaseAddr)[off]; }

    private:
        void *m_BaseAddr;
        size_t m_Size;
        CSuffixTree *m_SufTree;
};

#endif