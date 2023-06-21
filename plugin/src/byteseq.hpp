#ifndef H_BYTESEQ
#define H_BYTESEQ

#include <stdint.h>
#include <string.h>
#include <vector>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <functional>
#include "anchor.hpp"

class CSequentialByteSequence;

class IByteSequence {
    public:
        virtual ~IByteSequence() {}

        virtual bool has_data() const { return true; }
        virtual size_t size() const = 0;
        virtual const uint8_t *buffer() const { return nullptr; }

        virtual bool apply_anchor(SAnchor anchor) { return true; }

        virtual int compare(const IByteSequence &seq, size_t this_off, size_t seq_off, size_t size) const {
            if(!seq.has_data()) return seq.compare(*this, seq_off, this_off, size);
            const uint8_t *this_buf = buffer(), *seq_buf = seq.buffer();
            if( this_buf &&  seq_buf) return memcmp(this_buf + this_off, seq_buf + seq_off, size);
            if( this_buf && !seq_buf) return -seq.compare(this_buf + this_off, seq_off, size);
            if(!this_buf &&  seq_buf) return compare(seq_buf + seq_off, this_off, size);

            for(size_t i = 0; i < size; i++) {
                if((*this)[this_off + i] != seq[seq_off + i]) return ((*this)[this_off + i] < seq[seq_off + i]) ? -1 : 1;
            }
            return 0;
        }

        virtual int compare(const uint8_t *buf, size_t off, size_t size) const {
            const uint8_t *this_buf = buffer();
            if(this_buf) return memcmp(this_buf + off, buf, size);

            for(size_t i = 0; i < size; i++) {
                if((*this)[off + i] != buf[i]) return ((*this)[off + i] < buf[i]) ? -1 : 1;
            }
            return 0;
        }

        virtual void get_data(uint8_t *buf, size_t off, size_t size) const = 0;
        virtual uint8_t operator [](size_t off) const = 0;
};

class CByteSequenceWrapper : public IByteSequence {
    public:
        CByteSequenceWrapper(IByteSequence& wrapped_seq) : m_WrappedSeq(wrapped_seq) {}

        virtual bool has_data() const override { return m_WrappedSeq.has_data(); }
        virtual size_t size() const override { return m_WrappedSeq.size(); }
        virtual const uint8_t *buffer() const override { return m_WrappedSeq.buffer(); }

        virtual bool apply_anchor(SAnchor anchor) override { return m_WrappedSeq.apply_anchor(anchor); }

        virtual int compare(const IByteSequence &seq, size_t this_off, size_t seq_off, size_t size) const override { return m_WrappedSeq.compare(seq, this_off, seq_off, size); }
        virtual int compare(const uint8_t *buf, size_t off, size_t size) const override { return m_WrappedSeq.compare(buf, off, size); }

        virtual void get_data(uint8_t *buf, size_t off, size_t size) const override { return m_WrappedSeq.get_data(buf, off, size); }
        virtual uint8_t operator [](size_t off) const override { return m_WrappedSeq[off]; }

    private:
        IByteSequence& m_WrappedSeq;
};

class CSequentialByteSequence : public IByteSequence {
    public:
        CSequentialByteSequence() : m_Size(0) {}
        CSequentialByteSequence(std::initializer_list<IByteSequence*> seqs) : m_Size(0) {
            for(IByteSequence *seq : seqs) add_sequence(seq);
        }
        CSequentialByteSequence(CSequentialByteSequence &seq) : m_Size(seq.m_Size) { m_Sequences = std::move(seq.m_Sequences); }

        inline void add_sequence(IByteSequence *ptr, bool owned=true) {
            m_Sequences.push_back(SSeqEntry(m_Size, ptr, owned));
            m_Size += ptr->size();
        }

        template<typename SeqType, typename... Args> inline SeqType& emplace_sequence(Args&&... args) {
            SeqType *seq = new SeqType(args...);
            add_sequence(seq);
            return *seq;
        }

        virtual size_t size() const override { return m_Size; };

        virtual bool apply_anchor(SAnchor anchor) override;

        virtual int compare(const IByteSequence &seq, size_t this_off, size_t seq_off, size_t size) const override;
        virtual int compare(const uint8_t *buf, size_t off, size_t size) const override;
        virtual void get_data(uint8_t *buf, size_t off, size_t size) const override;

        inline virtual uint8_t operator [](size_t off) const override {
            const SSeqEntry& ent = get_sequence(off);
            return (*ent.seq)[off - ent.off];
        }

    private:
        struct SSeqEntry {
            size_t off;
            IByteSequence *seq;
            bool owns_seq;

            SSeqEntry() : off(0), seq(nullptr), owns_seq(false) {}
            SSeqEntry(size_t off, IByteSequence *seq, bool owns_seq) : off(off), seq(seq), owns_seq(owns_seq) {}
            SSeqEntry(SSeqEntry&& ent) : off(ent.off), seq(ent.seq), owns_seq(ent.owns_seq) { ent.owns_seq = false; }
            SSeqEntry(const SSeqEntry& ent) = delete;
            ~SSeqEntry() {
                if(owns_seq) delete seq;
                seq = nullptr;
            }
        };

        const SSeqEntry& get_sequence(size_t off) const;

        size_t m_Size;
        std::vector<SSeqEntry> m_Sequences;
};

class CIgnoreSequence : public IByteSequence {
    public:
        CIgnoreSequence(size_t size) : m_Size(size) {}

        virtual bool has_data() const override { return false; }
        virtual size_t size() const override { return m_Size; };

        virtual int compare(const IByteSequence &seq, size_t this_off, size_t seq_off, size_t size) const override { return 0; }
        virtual int compare(const uint8_t *buf, size_t off, size_t size) const override { return 0; }

        virtual void get_data(uint8_t *buf, size_t off, size_t size) const override { throw std::runtime_error("Can't access the data of an CIgnoreSequence"); }
        inline virtual uint8_t operator [](size_t off) const override { return 0x42; }

    private:
        size_t m_Size;
        uint8_t m_Fill;
};

class CFillSequence : public IByteSequence {
    public:
        CFillSequence(size_t size, uint8_t fill) : m_Size(size), m_Fill(fill) {}

        virtual size_t size() const override { return m_Size; };
        virtual void get_data(uint8_t *buf, size_t off, size_t size) const override { memset(buf, m_Fill, size); }
        inline virtual uint8_t operator [](size_t off) const override { return m_Fill; }

    private:
        size_t m_Size;
        uint8_t m_Fill;
};

class CBufferSequence : public IByteSequence {
    public:
        CBufferSequence(const uint8_t *data, size_t size) : m_Data(data), m_Size(size) {}

        virtual size_t size() const override { return m_Size; };
        virtual const uint8_t *buffer() const override { return m_Data; };
        virtual void get_data(uint8_t *buf, size_t off, size_t size) const override { memcpy(buf, m_Data+off, size); }
        virtual uint8_t operator [](size_t off) const override { return m_Data[off]; }

    private:
        const uint8_t *m_Data;
        size_t m_Size;
};

class CVectorSequence : public IByteSequence {
    public:
        CVectorSequence() {}
        CVectorSequence(std::vector<uint8_t> data) : m_Data(data) {}
        CVectorSequence(std::initializer_list<uint8_t> data) : m_Data(data) {}

        inline void push_back(uint8_t byte) { m_Data.push_back(byte); }

        virtual size_t size() const override { return m_Data.size(); };
        virtual const uint8_t *buffer() const override { return m_Data.data(); };
        virtual void get_data(uint8_t *buf, size_t off, size_t size) const override { memcpy(buf, m_Data.data()+off, size); }
        virtual uint8_t operator [](size_t off) const override { return m_Data[off]; }

    private:
        std::vector<uint8_t> m_Data;
};

class CHexSequence : public IByteSequence {
    public:
        CHexSequence(const char *hexstr, std::initializer_list<const void*> formats = std::initializer_list<const void*>());
        CHexSequence(const CHexSequence &seq);
        virtual ~CHexSequence();

        virtual size_t size() const override { return m_Size; };
        virtual const uint8_t *buffer() const override { return m_Data; };
        virtual void get_data(uint8_t *buf, size_t off, size_t size) const override { memcpy(buf, m_Data+off, size); }
        virtual uint8_t operator [](size_t off) const override { return m_Data[off]; }

    protected:
        uint8_t *m_Data;
        size_t m_Size;
};

class CMaskedHexSequence : public IByteSequence {
    public:
        CMaskedHexSequence(const char *hexstr);
        CMaskedHexSequence(const CMaskedHexSequence &seq);
        virtual ~CMaskedHexSequence();

        virtual bool has_data() const override { return false; }
        virtual size_t size() const override { return m_Size; };

        virtual int compare(const IByteSequence &seq, size_t this_off, size_t seq_off, size_t size) const override;
        virtual int compare(const uint8_t *buf, size_t off, size_t size) const override;

        virtual void get_data(uint8_t *buf, size_t off, size_t size) const override { throw std::runtime_error("Can't access the data of a CMaskedHexSequence"); }
        virtual uint8_t operator [](size_t off) const override { return 0x42; }

    protected:
        uint8_t *m_Data, *m_DataMask;
        size_t m_Size;
};

class CStringSequence : public IByteSequence {
    public:
        CStringSequence(const char *str) : m_String(str), m_Size(strlen(str)) {}

        virtual size_t size() const override { return m_Size; };
        virtual const uint8_t *buffer() const override { return (const uint8_t*) m_String; };
        virtual void get_data(uint8_t *buf, size_t off, size_t size) const override { memcpy(buf, m_String+off, size); }
        virtual uint8_t operator [](size_t off) const override { return (uint8_t) m_String[off]; }

    private:
        const char *m_String;
        size_t m_Size;
};

class CRefInstructionSequence : public IByteSequence {
    public:
        CRefInstructionSequence(std::vector<uint8_t> opcode, SAnchor anchor) : m_Opcode(opcode), m_RefAnchor(anchor), m_IsAnchored(false) {}

        virtual size_t size() const override { return m_Opcode.size() + sizeof(size_t); };

        virtual bool apply_anchor(SAnchor anchor) override {
            m_IsAnchored = true;
            m_InstrAnchor = anchor;
            return true;
        }

        virtual void get_data(uint8_t *buf, size_t off, size_t size) const override {
            while(size-- > 0) *(buf++) = (*this)[off++];
        }

        virtual uint8_t operator [](size_t off) const override {
            if(!m_IsAnchored) throw std::runtime_error("Can't access content of CRefInstructionSequence when not anchored");
            if(off < m_Opcode.size()) return m_Opcode[off];

            size_t rel = m_RefAnchor - (m_InstrAnchor + size());
            return ((uint8_t*) &rel)[off - m_Opcode.size()];
        }

    private:
        std::vector<uint8_t> m_Opcode;
        uint64_t m_OpcodeBytes;
        SAnchor m_RefAnchor;

        bool m_IsAnchored;
        SAnchor m_InstrAnchor;
};

#define SEQ_WRAP(seq) CByteSequenceWrapper(seq)
#define SEQ_SEQ(...) CSequentialByteSequence({ __VA_ARGS__ })
#define SEQ_IGN(len) CIgnoreSequence(len, val)
#define SEQ_FILL(len, val) CFillSequence(len, val)
#define SEQ_BUF(ptr, size) CBufferSequence(ptr, size)
#define SEQ_HEX(str, ...) CHexSequence(str, { __VA_ARGS__ })
#define SEQ_MASKED_HEX(str) CMaskedHexSequence(str)
#define SEQ_STR(str) CStringSequence(str)
#define SEQ_CALL(ref) CRefInstructionSequence({ 0xe8 }, ref)
#define SEQ_JMP(ref) CRefInstructionSequence({ 0xe9 }, ref)
#define SEQ_JZ(ref)  CRefInstructionSequence({ 0x0f, 0x84 }, ref)
#define SEQ_JNZ(ref) CRefInstructionSequence({ 0x0f, 0x85 }, ref)
#define SEQ_JL(ref)  CRefInstructionSequence({ 0x0f, 0x8c }, ref)
#define SEQ_JNL(ref) CRefInstructionSequence({ 0x0f, 0x8d }, ref)
#define SEQ_JG(ref)  CRefInstructionSequence({ 0x0f, 0x8f }, ref)
#define SEQ_JNG(ref) CRefInstructionSequence({ 0x0f, 0x8e }, ref)
#define SEQ_JLE(ref) SEQ_JNG(ref)
#define SEQ_JGE(ref) SEQ_JNL(ref)

#endif