#ifndef H_BYTESEQ
#define H_BYTESEQ

#include <stdint.h>
#include <string.h>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <functional>
#include "anchor.hpp"

class CSequentialByteSequence;

class IByteSequence {
    public:
        virtual size_t size() const = 0;
        virtual const uint8_t *buffer() const { return nullptr; }

        virtual bool set_anchor(SAnchor anchor) { return true; }

        virtual int compare(const IByteSequence &seq, size_t this_off, size_t seq_off, size_t size) const {
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
            if(this_buf) return memcmp(this_buf + off, buf, size) == 0;

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
        CSequentialByteSequence(std::initializer_list<IByteSequence*> seqs);
        CSequentialByteSequence(CSequentialByteSequence &seq);
        ~CSequentialByteSequence();

        virtual size_t size() const { return m_Size; };

        virtual bool set_anchor(SAnchor anchor);

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
            IByteSequence* seq;

            inline seq_ent() : off(0), seq(nullptr) {}
            inline seq_ent(size_t off, IByteSequence* seq) : off(off), seq(seq) {}
        };

        const seq_ent& get_sequence(size_t off) const;

        size_t m_Size;
        int m_NumSeqs;
        seq_ent *m_Sequences;
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

class CHexSequence : public IByteSequence {
    public:
        CHexSequence(const char *hexstr, std::initializer_list<const void*> formats = std::initializer_list<const void*>());
        CHexSequence(const CHexSequence &seq);
        ~CHexSequence();

        virtual size_t size() const { return m_Size; };
        virtual const uint8_t *buffer() const { return m_Data; };
        virtual void get_data(uint8_t *buf, size_t off, size_t size) const { memcpy(buf, m_Data+off, size); }
        virtual uint8_t operator [](size_t off) const { return m_Data[off]; }

    private:
        uint8_t *m_Data;
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
        CRefInstructionSequence(const IByteSequence& opcode, SAnchor anchor) : m_Opcode(opcode), m_RefAnchor(anchor), m_IsAnchored(false) {}

        virtual size_t size() const { return m_Opcode.size() + sizeof(size_t); };

        virtual bool set_anchor(SAnchor anchor) {
            m_InstrAnchor = anchor;
            return true;
        }

        virtual void get_data(uint8_t *buf, size_t off, size_t size) const {
            while(size-- > 0) *(buf++) = (*this)[off++];
        }

        virtual uint8_t operator [](size_t off) const {
            if(!m_IsAnchored) throw std::runtime_error("Can't access content of CRefInstructionSequence when not anchored");
            if(off < m_Opcode.size()) return m_Opcode[off];

            size_t rel = m_RefAnchor + size() - m_InstrAnchor;
            return ((uint8_t*) &rel)[off-1];
        }

    private:
        const IByteSequence& m_Opcode;
        SAnchor m_RefAnchor;

        bool m_IsAnchored;
        SAnchor m_InstrAnchor;
};

#define SEQ_SEQ(...) CSequentialByteSequence({ __VA_ARGS__ })
#define SEQ_HEX(str) CHexSequence(str)
#define SEQ_STR(str) CStringSequence(str)
#define SEQ_JMP(ref) CRefInstructionSequence(CArraySequence((const uint8_t[]) { 0xe9 }, 1), ref)
#define SEQ_JZ(ref)  CRefInstructionSequence(CArraySequence((const uint8_t[]) { 0x0f, 0x84 }, 2), ref)
#define SEQ_JNZ(ref) CRefInstructionSequence(CArraySequence((const uint8_t[]) { 0x0f, 0x85 }, 2), ref)
#define SEQ_JL(ref)  CRefInstructionSequence(CArraySequence((const uint8_t[]) { 0x0f, 0x8c }, 2), ref)
#define SEQ_JNL(ref) CRefInstructionSequence(CArraySequence((const uint8_t[]) { 0x0f, 0x8d }, 2), ref)
#define SEQ_JG(ref)  CRefInstructionSequence(CArraySequence((const uint8_t[]) { 0x0f, 0x8f }, 2), ref)
#define SEQ_JNG(ref) CRefInstructionSequence(CArraySequence((const uint8_t[]) { 0x0f, 0x8e }, 2), ref)
#define SEQ_JLE(ref) SEQ_JNG(ref)
#define SEQ_JGE(ref) SEQ_JNL(ref)

#endif