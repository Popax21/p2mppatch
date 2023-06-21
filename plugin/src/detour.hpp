#ifndef H_P2MPPATCH_DETOUR
#define H_P2MPPATCH_DETOUR

#include <tier0/dbg.h>
#include <tier0/valve_minmax_off.h>

#include "byteseq.hpp"
#include "scratchpad.hpp"
#include "patch.hpp"

class CScratchDetourSeq : public IByteSequence {
    public:
        static const int MIN_SIZE = 5;

        CScratchDetourSeq(CScratchPad& scratch, int size, IByteSequence *scratch_seq, bool jump_back = true) : CScratchDetourSeq(scratch, size) {
            m_DetourSeq = std::unique_ptr<IByteSequence>(scratch_seq);
            m_DetourJumpBack = jump_back;
        }

        const CScratchPad::SSeqEntry& scratch_entry() const { return m_ScratchPadEntry; }

        virtual bool has_data() const override {
            check_has_seq();
            return m_ScratchJumpSeq->has_data();
        }

        virtual size_t size() const override {
            return m_DetourSize;
        }

        virtual const uint8_t *buffer() const override {
            check_has_seq();
            return m_ScratchJumpSeq->buffer();
        }

        virtual void apply_anchor(SAnchor anchor) override {
            update_detour_anchor(anchor);
            if(m_ScratchJumpSeq) m_ScratchJumpSeq->apply_anchor(anchor);
            m_DetourAnchor = anchor;
        }

        virtual int compare(const IByteSequence &seq, size_t this_off, size_t seq_off, size_t size) const override {
            check_has_seq();
            return m_ScratchJumpSeq->compare(seq, this_off, seq_off, size);
        }

        virtual int compare(const uint8_t *buf, size_t off, size_t size) const override {
            check_has_seq();
            return m_ScratchJumpSeq->compare(buf, off, size);
        }

        virtual void get_data(uint8_t *buf, size_t off, size_t size) const override {
            check_has_seq();
            m_ScratchJumpSeq->get_data(buf, off, size);
        }
    
        virtual uint8_t operator [](size_t off) const override {
            check_has_seq();
            return (*m_ScratchJumpSeq)[off];
        }

    protected:
        CScratchDetourSeq(CScratchPad& scratch, int size) : m_ScratchPad(scratch), m_DetourSize(size), m_DetourSeq(nullptr), m_DetourJumpBack(false), m_ScratchJumpSeq(nullptr) {
            if(size < MIN_SIZE) throw std::runtime_error("Invalid detour patchsite size");
        }

        inline SAnchor cur_anchor() const { return m_DetourAnchor; }
        virtual void update_detour_anchor(SAnchor anchor);

        SAnchor set_scratch_seq(IByteSequence& seq);

    private:
        CScratchPad& m_ScratchPad;
        int m_DetourSize;
        std::unique_ptr<IByteSequence> m_DetourSeq;
        bool m_DetourJumpBack;

        CScratchPad::SSeqEntry m_ScratchPadEntry;
        std::unique_ptr<IByteSequence> m_ScratchJumpSeq;
        SAnchor m_DetourAnchor;

        inline void check_has_seq() const {
            if(m_ScratchJumpSeq == nullptr) throw std::runtime_error("CScratchDetour has not been given a scratch sequence yet");
        }
};

class CFuncDetour : public CScratchDetourSeq {
    public:
        struct SArgument {
            enum EType {
                REG, LOCAL_VAR, STACK_VAR
            };

            EType type;
            int val;

            SArgument(EType type, int val) : type(type), val(val) {}

            inline bool operator ==(SArgument other) const { return type == other.type && val == other.val; }
            inline bool operator !=(SArgument other) const { return !(*this == other); }

            static const SArgument reg_eax, reg_ebx, reg_ecx, reg_edx, reg_esp, reg_ebp, reg_esi, reg_edi, reg_eip;

            static inline SArgument local_var(int ebp_off) { return SArgument(EType::LOCAL_VAR, ebp_off); };
            static inline SArgument stack_var(int esp_off) { return SArgument(EType::STACK_VAR, esp_off); };
        };

        CFuncDetour(CScratchPad& scratch, int size, void *func, std::initializer_list<SArgument> args) : CScratchDetourSeq(scratch, size), m_DetourFunc(func), m_DetourArgs(args) {}

        CFuncDetour *prepend_orig_prefix() {
            m_PrefixSeqs.push_back(std::unique_ptr<IByteSequence>(nullptr));
            reapply_anchor();
            return this;
        }

        CFuncDetour *prepend_seq_prefix(IByteSequence *seq) {
            m_PrefixSeqs.emplace_back(seq);
            reapply_anchor();
            return this;
        }

        CFuncDetour *append_orig_suffix() {
            m_SuffixSeqs.push_back(std::unique_ptr<IByteSequence>(nullptr));
            reapply_anchor();
            return this;
        }

        CFuncDetour *append_seq_suffix(IByteSequence *seq) {
            m_SuffixSeqs.emplace_back(seq);
            reapply_anchor();
            return this;
        }

    protected:
        virtual void update_detour_anchor(SAnchor anchor) override;

    private:
        inline void reapply_anchor() {
            if(cur_anchor()) apply_anchor(cur_anchor());
        }

        void *m_DetourFunc;
        const std::vector<SArgument> m_DetourArgs;
        std::vector<std::unique_ptr<IByteSequence>> m_PrefixSeqs, m_SuffixSeqs;
};

#define DETOUR_FUNC __attribute__((force_align_arg_pointer)) __attribute__((cdecl))

#define DETOUR_ARG_EAX CFuncDetour::SArgument::reg_eax
#define DETOUR_ARG_EBX CFuncDetour::SArgument::reg_ebx
#define DETOUR_ARG_ECX CFuncDetour::SArgument::reg_ecx
#define DETOUR_ARG_EDX CFuncDetour::SArgument::reg_edx
#define DETOUR_ARG_ESP CFuncDetour::SArgument::reg_esp
#define DETOUR_ARG_EBP CFuncDetour::SArgument::reg_ebp
#define DETOUR_ARG_ESI CFuncDetour::SArgument::reg_esi
#define DETOUR_ARG_EDI CFuncDetour::SArgument::reg_edi
#define DETOUR_ARG_EIP CFuncDetour::SArgument::reg_eip
#define DETOUR_ARG_LOCAL(ebp_off) CFuncDetour::SArgument::local_var(ebp_off)
#define DETOUR_ARG_STACK(esp_off) CFuncDetour::SArgument::stack_var(esp_off)

#define SEQ_SCRATCH_DETOUR(plugin, size, seq) CScratchDetourSeq(plugin.scratchpad(), size, seq)
#define SEQ_SCRATCH_DETOUR_NRET(plugin, size, seq) CScratchDetourSeq(plugin.scratchpad(), size, seq, false)
#define SEQ_FUNC_DETOUR(plugin, size, func, ...) CFuncDetour(plugin.scratchpad(), size, (void*) func, { __VA_ARGS__ })

#endif