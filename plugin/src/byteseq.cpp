#include <stdexcept>
#include "byteseq.hpp"

void CSequentialByteSequence::apply_anchor(SAnchor anchor) {
    for(int i = 0; i < m_Sequences.size(); i++) {
        const SSeqEntry& ent = m_Sequences[i];
        ent.seq->apply_anchor(anchor + ent.off);
    }
}

int CSequentialByteSequence::compare(const IByteSequence &seq, size_t this_off, size_t seq_off, size_t size) const {
    while(size > 0) {
        const SSeqEntry& seq_ent = get_sequence(this_off);

        size_t sz = std::min(seq_ent.seq->size(), size);
        int r = seq_ent.seq->compare(seq, this_off - seq_ent.off, seq_off, sz);
        if(r != 0) return r;

        this_off += sz;
        seq_off += sz;
        size -= sz;
    }
    return 0;
}

int CSequentialByteSequence::compare(const uint8_t *buf, size_t off, size_t size) const {
    while(size > 0) {
        const SSeqEntry& seq_ent = get_sequence(off);

        size_t sz = std::min(seq_ent.seq->size(), size);
        int r = seq_ent.seq->compare(buf, off - seq_ent.off, sz);
        if(r != 0) return r;

        buf += sz;
        off += sz;
        size -= sz;
    }
    return 0;
}

void CSequentialByteSequence::get_data(uint8_t *buf, size_t off, size_t size) const {
    while(size > 0) {
        const SSeqEntry& seq_ent = get_sequence(off);

        size_t sz = std::min(seq_ent.seq->size(), size);
        seq_ent.seq->get_data(buf, off - seq_ent.off, sz);

        buf += sz;
        off += sz;
        size -= sz;
    }
}

const CSequentialByteSequence::SSeqEntry& CSequentialByteSequence::get_sequence(size_t off) const {
    int s = 0, e = m_Sequences.size();
    while(s < e-1) {
        int m = s + (e-s) / 2;
        if(off < m_Sequences[m].off) e = m;
        else s = m;
    }
    return m_Sequences[s];
}

static uint8_t hex_lut[256] = { 0 };

static inline void init_hex_lut() {
    static bool is_initalized = false;
    if(is_initalized) return;
 
    memset(hex_lut, 0, sizeof(hex_lut));
    for(char chr = '0'; chr <= '9'; chr++) hex_lut[chr] = chr - '0';
    for(char chr = 'a'; chr <= 'f'; chr++) hex_lut[chr] = chr - 'a' + 0xa;
    for(char chr = 'A'; chr <= 'F'; chr++) hex_lut[chr] = chr - 'A' + 0xa;

    is_initalized = true;
}

CHexSequence::CHexSequence(const char *hexstr, std::initializer_list<const void*> formats) : m_Size(0) {
    //Determine sequence size
    std::initializer_list<const void*>::const_iterator formats_iter = formats.begin();
    for(const char *p = hexstr; *p;) {
        if(isspace(*p)) {
            p++;
            continue;
        }

        //Format specifiers
        if(*p == '$') {
            if(*(p+1) == '$') {
                m_Size += ((IByteSequence*) *formats_iter)->size();
            } else {
                m_Size += strtol(p+1, (char**) &p, 10);
            }
            formats_iter++;
            continue;
        }

        //Regular hex char
        m_Size++;
        p += 2;
    }

    //Create sequence data
    m_Data = new uint8_t[m_Size];

    //Determine data
    init_hex_lut();

    uint8_t *data_ptr = m_Data;
    formats_iter = formats.begin();
    for(const char *p = hexstr; *p;) {
        if(isspace(*p)) {
            p++;
            continue;
        }

        //Format specifiers
        if(*p == '$') {
            if(*(p+1) == '$') {
                //Get sequence data
                IByteSequence *seq = (IByteSequence*) *formats_iter;
                seq->get_data(data_ptr, 0, seq->size());
                data_ptr += seq->size();
            } else {
                //Copy buffer provided in formats varars
                size_t sz = strtol(p+1, (char**) &p, 10);
                memcpy(data_ptr, *formats_iter, sz);
                data_ptr += sz;
            }
            formats_iter++;
            continue;
        }

        //Regular hex char
        *(data_ptr++) = (hex_lut[*p] << 4) | hex_lut[*(p+1)];
        p += 2;
    }
}

CHexSequence::CHexSequence(const CHexSequence &seq) {
    m_Data = new uint8_t[m_Size = seq.m_Size];
    memcpy(m_Data, seq.m_Data, m_Size);
}

CHexSequence::~CHexSequence() {
    delete[] m_Data;
    m_Data = nullptr;
}

CMaskedHexSequence::CMaskedHexSequence(const char *hexstr) : m_Size(0) {
    //Determine sequence size
    for(const char *p = hexstr; *p;) {
        if(isspace(*p)) {
            p++;
        } else {
            m_Size++;
            p += 2;
        }
    }

    //Create sequence data
    m_Data = new uint8_t[m_Size];
    m_DataMask = new uint8_t[m_Size];

    //Determine data
    init_hex_lut();

    uint8_t *data_ptr = m_Data, *mask_ptr = m_DataMask;
    for(const char *p = hexstr; *p;) {
        if(isspace(*p)) {
            p++;
        } else {
            *(data_ptr++) = (hex_lut[*p] << 4) | hex_lut[*(p+1)];
            *(mask_ptr++) = ((*p == '?' ? 0x0 : 0xf) << 4) | (*(p+1) == '?' ? 0x0 : 0xf);
            p += 2;
        }
    }
}

CMaskedHexSequence::CMaskedHexSequence(const CMaskedHexSequence &seq) {
    m_Data = new uint8_t[m_Size = seq.m_Size];
    m_DataMask = new uint8_t[m_Size];
    memcpy(m_Data, seq.m_Data, m_Size);
    memcpy(m_DataMask, seq.m_DataMask, m_Size);
}

CMaskedHexSequence::~CMaskedHexSequence() {
    delete[] m_Data;
    delete[] m_DataMask;
    m_Data = nullptr;
    m_DataMask = nullptr;
}

int CMaskedHexSequence::compare(const IByteSequence &seq, size_t this_off, size_t seq_off, size_t size) const {
    for(; size > 0; this_off++, seq_off++, size--) {
        uint8_t mask = m_DataMask[this_off];
        if((m_Data[this_off] & mask) != (seq[seq_off] & mask)) return (m_Data[this_off] & mask) < (seq[seq_off] & mask) ? -1 : +1;
    }
    return 0;
}

int CMaskedHexSequence::compare(const uint8_t *buf, size_t off, size_t size) const {
    for(; size > 0; buf++, off++, size--) {
        uint8_t mask = m_DataMask[off];
        if((m_Data[off] & mask) != (*buf & mask)) return (m_Data[off] & mask) < (*buf & mask) ? -1 : +1;
    }
    return 0;
}
