#ifndef H_P2MPPATCH_DETOUR
#define H_P2MPPATCH_DETOUR

#include "byteseq.hpp"
#include "scratchpad.hpp"
#include "patch.hpp"

#define DETOUR_FUNC __attribute__((force_align_arg_pointer)) __attribute__((cdecl))

class CDetour : public IByteSequence {
    public:
        struct SArgument {
            bool is_reg;
            int val;

            SArgument(bool is_reg, int val) : is_reg(is_reg), val(val) {}

            static const SArgument REG_EAX, REG_EBX, REG_ECX, REG_EDX, REG_ESI, REG_EDI;

            static inline SArgument local_var(int ebp_off) { return SArgument(false, ebp_off); };
        };

        static const int MIN_SIZE = 5;

        CDetour(CScratchPad& scratch, int size, void *func, std::initializer_list<SArgument> args) : m_ScratchPad(scratch), m_DetourSize(size), m_DetourFunc(func), m_DetourArgs(args) {
            if(size < MIN_SIZE) throw std::runtime_error("Invalid detour patchsite size!");
        }
        virtual ~CDetour() {}

        virtual size_t size() const { return m_DetourSize; }
        virtual const uint8_t *buffer() const {
            if(m_DetourJumpSeq == nullptr) throw std::runtime_error("CDetour has not been anchored yet!");
            return m_DetourJumpSeq->buffer();
        }

        virtual bool apply_anchor(SAnchor anchor);

        virtual int compare(const IByteSequence &seq, size_t this_off, size_t seq_off, size_t size) const {
            if(m_DetourJumpSeq == nullptr) throw std::runtime_error("CDetour has not been anchored yet!");
            return m_DetourJumpSeq->compare(seq, this_off, seq_off, size);
        }

        virtual int compare(const uint8_t *buf, size_t off, size_t size) const {
            if(m_DetourJumpSeq == nullptr) throw std::runtime_error("CDetour has not been anchored yet!");
            return m_DetourJumpSeq->compare(buf, off, size);
        }

        virtual void get_data(uint8_t *buf, size_t off, size_t size) const {
            if(m_DetourJumpSeq == nullptr) throw std::runtime_error("CDetour has not been anchored yet!");
            m_DetourJumpSeq->get_data(buf, off, size);
        }

        inline virtual uint8_t operator [](size_t off) const {
            if(m_DetourJumpSeq == nullptr) throw std::runtime_error("CDetour has not been anchored yet!");
            return (*m_DetourJumpSeq)[off];
        }

    private:
        CScratchPad& m_ScratchPad;
        int m_DetourSize;
        void *m_DetourFunc;
        const std::vector<SArgument> m_DetourArgs;

        CScratchPad::SSeqEntry m_ScratchPadEntry;
        std::unique_ptr<IByteSequence> m_DetourJumpSeq;
};

#define DETOUR_ARG_EAX CDetour::SArgument::REG_EAX
#define DETOUR_ARG_EBX CDetour::SArgument::REG_EBX
#define DETOUR_ARG_ECX CDetour::SArgument::REG_ECX
#define DETOUR_ARG_EDX CDetour::SArgument::REG_EDX
#define DETOUR_ARG_ESI CDetour::SArgument::REG_ESI
#define DETOUR_ARG_EDI CDetour::SArgument::REG_EDI
#define DETOUR_ARG_LOCAL(ebp_off) CDetour::SArgument::local_var(ebp_off)

#define SEQ_DETOUR(plugin, size, func, ...) CDetour(plugin.scratchpad(), size, (void*) func, { })

#endif