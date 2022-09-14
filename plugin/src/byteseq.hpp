#ifndef H_BYTESEQ
#define H_BYTESEQ

#include <stdint.h>
#include <string.h>

class IByteSequence {
    public:
        virtual size_t size() const = 0;
        virtual const uint8_t *buffer() const { return nullptr; }

        virtual bool compare(const IByteSequence &seq, size_t this_off, size_t seq_off, size_t size) const {
            const uint8_t *this_buf = buffer(), *seq_buf = seq.buffer();
            if( this_buf &&  seq_buf) return memcmp(this_buf + this_off, seq_buf + seq_off, size) == 0;
            if( this_buf && !seq_buf) return seq.compare(this_buf + this_off, seq_off, size);
            if(!this_buf &&  seq_buf) return compare(seq_buf + seq_off, this_off, size);

            for(size_t i = 0; i < size; i++) {
                if((*this)[this_off + i] != seq[seq_off + i]) return false;
            }
            return true;
        }

        virtual bool compare(const uint8_t *buf, size_t off, size_t size) const {
            const uint8_t *this_buf = buffer();
            if(this_buf) return memcmp(this_buf + off, buf, size) == 0;

            for(size_t i = 0; i < size; i++) {
                if((*this)[off + i] != buf[i]) return false;
            }
            return true;
        }

        virtual void get_data(uint8_t *buf, size_t off, size_t size) const = 0;
        virtual uint8_t operator [](size_t off) const = 0;
};

class CHexSequence : public IByteSequence {
    public:
        CHexSequence(const char *hexstr);
        CHexSequence(const CHexSequence &seq);
        ~CHexSequence();

        virtual size_t size() const { return m_Size; };
        virtual const uint8_t *buffer() const { return m_Data; };
        virtual void get_data(uint8_t *buf, size_t off, size_t size) const { memcpy(buf, m_Data+off, size); }
        virtual uint8_t operator [](size_t off) const { return m_Data[off]; }

    private:
        size_t m_Size;
        uint8_t *m_Data;
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

#endif