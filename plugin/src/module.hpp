#ifndef H_P2MPPATCH_MODULE
#define H_P2MPPATCH_MODULE

#include <tier0/dbg.h>
#include <tier0/valve_minmax_off.h>

#include <memory>
#include <sstream>
#include <stdexcept>
#include "byteseq.hpp"
#include "sufarr.hpp"
#include "anchor.hpp"

class CModule;

class IModuleDependent {
    protected:
        ~IModuleDependent() {}

        virtual void associate(CModule& module) = 0;
        virtual void reset() = 0;

        friend class CModule;
};

class CModule : public IByteSequence {
    public:
        static const int PAGE_SIZE = 0x1000;
        static const int MAX_NEEDLE_SIZE = 32;

        class CUnprotector {
            public:
                CUnprotector(CModule *mod) : m_Module(mod) {
                    if(mod) mod->unprotect();
                }
                CUnprotector(CUnprotector&& prot) = delete;
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
        CModule(CModule&& mod) = delete;
        CModule(const CModule& mod) = delete;
        ~CModule();

        virtual const char *name() const { return m_Name; }
        virtual void *base_addr() const { return m_BaseAddr; }
        virtual size_t size() const { return m_Size; };

        virtual int compare(const IByteSequence &seq, size_t this_off, size_t seq_off, size_t size) const;
        virtual int compare(const uint8_t *buf, size_t off, size_t size) const;
        virtual void get_data(uint8_t *buf, size_t off, size_t size) const;

        virtual SAnchor find_seq_anchor(const IByteSequence &seq, IByteSequence *discrim_seq = nullptr, ssize_t discrim_off = 0) const;

        virtual void unprotect();
        virtual void reprotect();

        virtual void associate_dependent(IModuleDependent *dep);
        virtual void reset_dependents();

        virtual uint8_t operator [](size_t off) const {
            if(!(m_PageFlags[off / PAGE_SIZE] & (int) EPageFlag::PAGE_R)) return 0;
            return ((uint8_t*) m_BaseAddr)[off];
        }

    private:
        const char *m_Name;
        void *m_BaseAddr;
        size_t m_Size;

        std::unique_ptr<uint8_t[]> m_PageFlags;
        CSuffixArray *m_SufArray;

        std::vector<IModuleDependent*> m_Dependents;
};

template<typename T> class IModuleFact : IModuleDependent {
    public:
        virtual const char *fact_name() const = 0;

        T const & get(CModule& module) {
            if(!m_Module) module.associate_dependent(this);
            if(m_Module == &module) return m_Value;

            std::stringstream sstream;
            sstream << "Conflicting IModuleFact module associations: '" << m_Module->name() << "' (" << m_Module << ") != '" << module.name() << "' (" << &module << ")";
            throw std::runtime_error(sstream.str());
        }

    protected:
        IModuleFact() : m_Module(nullptr) {}
        ~IModuleFact() {}

        virtual T determine_value(CModule& module) = 0;

    private:
        CModule *m_Module;
        T m_Value;

        virtual void associate(CModule& module) override {
            m_Value = determine_value(module);
            m_Module = &module;
            DevMsg("Associated IModuleFact '%s' with module '%s'\n", fact_name(), m_Module->name());
        }

        virtual void reset() override {
            m_Module = nullptr;
            m_Value = T();
        }
};

#endif