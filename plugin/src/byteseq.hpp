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

class CSequentialByteSequence : public IByteSequence {
    public:
        CSequentialByteSequence() : m_Size(0) {}
        CSequentialByteSequence(std::initializer_list<IByteSequence*> seqs) : m_Size(0) {
            for(IByteSequence *seq : seqs) add_sequence(seq);
        }
        CSequentialByteSequence(CSequentialByteSequence &seq) : m_Size(seq.m_Size) { m_Sequences = std::move(seq.m_Sequences); }
        ~CSequentialByteSequence() {}

        void add_sequence(IByteSequence* ptr) {
            m_Sequences.emplace_back(m_Size, std::unique_ptr<IByteSequence>(ptr));
            m_Size += ptr->size();
        }

        template<typename SeqType, typename... Args> SeqType& emplace_sequence(Args... args) {
            SeqType *seq = new SeqType(args...);
            add_sequence(seq);
            return *seq;
        }

        virtual size_t size() const { return m_Size; };

        virtual bool apply_anchor(SAnchor anchor);

        virtual int compare(const IByteSequence &seq, size_t this_off, size_t seq_off, size_t size) const;
        virtual int compare(const uint8_t *buf, size_t off, size_t size) const;
        virtual void get_data(uint8_t *buf, size_t off, size_t size) const;

        inline virtual uint8_t operator [](size_t off) const {
            const seq_ent& ent = get_sequence(off);
            return (*ent.seq)[off - ent.off];
        }

    private:
        struct seq_ent {
            size_t off;
            std::unique_ptr<IByteSequence> seq;

            inline seq_ent() : off(0), seq(nullptr) {}
            inline seq_ent(size_t off, std::unique_ptr<IByteSequence> seq) : off(off), seq(std::move(seq)) {}
        };

        const seq_ent& get_sequence(size_t off) const;

        size_t m_Size;
        std::vector<seq_ent> m_Sequences;
};

class CIgnoreSequence : public IByteSequence {
    public:
        CIgnoreSequence(size_t size) : m_Size(size) {}

        virtual bool has_data() const { return false; }
        virtual size_t size() const { return m_Size; };

        virtual int compare(const IByteSequence &seq, size_t this_off, size_t seq_off, size_t size) const { return 0; }
        virtual int compare(const uint8_t *buf, size_t off, size_t size) const { return 0; }

        virtual void get_data(uint8_t *buf, size_t off, size_t size) const { throw std::runtime_error("Can't access the data of an CIgnoreSequence"); }
        inline virtual uint8_t operator [](size_t off) const { return 0x42; }

    private:
        size_t m_Size;
        uint8_t m_Fill;
};

class CFillSequence : public IByteSequence {
    public:
        CFillSequence(size_t size, uint8_t fill) : m_Size(size), m_Fill(fill) {}

        virtual size_t size() const { return m_Size; };
        virtual void get_data(uint8_t *buf, size_t off, size_t size) const { memset(buf, m_Fill, size); }
        inline virtual uint8_t operator [](size_t off) const{ return m_Fill; }

    private:
        size_t m_Size;
        uint8_t m_Fill;
};

class CArraySequence : public IByteSequence {
    public:
        CArraySequence(const uint8_t *data, size_t size) : m_Data(data), m_Size(size) {}

        virtual size_t size() const { return m_Size; };
        virtual const uint8_t *buffer() const { return m_Data; };
        virtual void get_data(uint8_t *buf, size_t off, size_t size) const { memcpy(buf, m_Data+off, size); }
        virtual uint8_t operator [](size_t off) const { return m_Data[off]; }

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

        virtual size_t size() const { return m_Data.size(); };
        virtual const uint8_t *buffer() const { return m_Data.data(); };
        virtual void get_data(uint8_t *buf, size_t off, size_t size) const { memcpy(buf, m_Data.data()+off, size); }
        virtual uint8_t operator [](size_t off) const { return m_Data[off]; }

    private:
        std::vector<uint8_t> m_Data;
};

class CHexSequence : public IByteSequence {
    public:
        CHexSequence(const char *hexstr, std::initializer_list<const void*> formats = std::initializer_list<const void*>());
        CHexSequence(const CHexSequence &seq);
        virtual ~CHexSequence();

        virtual size_t size() const { return m_Size; };
        virtual const uint8_t *buffer() const { return m_Data; };
        virtual void get_data(uint8_t *buf, size_t off, size_t size) const { memcpy(buf, m_Data+off, size); }
        virtual uint8_t operator [](size_t off) const { return m_Data[off]; }

    protected:
        uint8_t *m_Data;
        size_t m_Size;
};

class CMaskedHexSequence : public IByteSequence {
    public:
        CMaskedHexSequence(const char *hexstr);
        CMaskedHexSequence(const CMaskedHexSequence &seq);
        virtual ~CMaskedHexSequence();

        virtual bool has_data() const { return false; }
        virtual size_t size() const { return m_Size; };

        virtual int compare(const IByteSequence &seq, size_t this_off, size_t seq_off, size_t size) const;
        virtual int compare(const uint8_t *buf, size_t off, size_t size) const;

        virtual void get_data(uint8_t *buf, size_t off, size_t size) const { throw std::runtime_error("Can't access the data of an CMaskedHexSequence"); }
        virtual uint8_t operator [](size_t off) const { return 0x42; }

    protected:
        uint8_t *m_Data, *m_DataMask;
        size_t m_Size;
};

class CStringSequence : public IByteSequence {
    public:
        CStringSequence(const char *str) : m_String(str), m_Size(strlen(str)) {}

        virtual size_t size() const { return m_Size; };
        virtual const uint8_t *buffer() const { return (const uint8_t*) m_String; };
        virtual void get_data(uint8_t *buf, size_t off, size_t size) const { memcpy(buf, m_String+off, size); }
        virtual uint8_t operator [](size_t off) const { return (uint8_t) m_String[off]; }

    private:
        const char *m_String;
        size_t m_Size;
};

class CRefInstructionSequence : public IByteSequence {
    public:
        CRefInstructionSequence(std::vector<uint8_t> opcode, SAnchor anchor) : m_Opcode(opcode), m_RefAnchor(anchor), m_IsAnchored(false) {}

        virtual size_t size() const { return m_Opcode.size() + sizeof(size_t); };

        virtual bool apply_anchor(SAnchor anchor) {
            if(m_IsAnchored) throw std::runtime_error("CRefInstructionSequence is already anchored");
            m_IsAnchored = true;
            m_InstrAnchor = anchor;
            return true;
        }

        virtual void get_data(uint8_t *buf, size_t off, size_t size) const {
            while(size-- > 0) *(buf++) = (*this)[off++];
        }

        virtual uint8_t operator [](size_t off) const {
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

#define SEQ_SEQ(...) CSequentialByteSequence({ __VA_ARGS__ })
#define SEQ_IGN(len) CIgnoreSequence(len, val)
#define SEQ_FILL(len, val) CFillSequence(len, val)
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