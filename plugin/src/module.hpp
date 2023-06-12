#ifndef H_P2MPPATCH_MODULE
#define H_P2MPPATCH_MODULE

#include "byteseq.hpp"
#include "sufarr.hpp"
#include "anchor.hpp"

#define PAGE_SIZE 0x1000

class CModule : public IByteSequence {
    public:
        static const int MAX_NEEDLE_SIZE = 32;

        class CUnprotector {
            public:
                CUnprotector(CModule *mod) : m_Module(mod) {
                    if(mod) mod->unprotect();
                }
                CUnprotector(CUnprotector& prot) = delete;
                CUnprotector(const CUnprotector& prot) = delete;
                ~CUnprotector() { finalize(); }

                void finalize() {
                    if(!m_Module) return;
                    m_Module->reprotect();
                    m_Module = nullptr;
                }

            private:
                CModule *m_Module;
        };

        enum class EPageFlag {
            PAGE_R = 1, PAGE_W = 2, PAGE_X = 4, PAGE_UNPROT = 8
        };

        CModule(const char *name);
        CModule(CModule& mod) = delete;
        CModule(const CModule& mod) = delete;
        ~CModule();

        virtual const char *name() const { return m_Name; }
        virtual void *base_addr() const { return m_BaseAddr; }
        virtual size_t size() const { return m_Size; };

        virtual int compare(const IByteSequence &seq, size_t this_off, size_t seq_off, size_t size) const;
        virtual int compare(const uint8_t *buf, size_t off, size_t size) const;
        virtual void get_data(uint8_t *buf, size_t off, size_t size) const;

        virtual SAnchor find_seq_anchor(const IByteSequence &seq) const;

        virtual void unprotect();
        virtual void reprotect();

        virtual uint8_t operator [](size_t off) const {
            if(!(m_PageFlags[off / PAGE_SIZE] & (int) EPageFlag::PAGE_R)) return 0;
            return ((uint8_t*) m_BaseAddr)[off];
        }

    private:
        const char *m_Name;
        void *m_BaseAddr;
        size_t m_Size;

        uint8_t *m_PageFlags;
        CSuffixArray *m_SufArray;
};

#endif