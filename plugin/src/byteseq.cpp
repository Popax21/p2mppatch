#include <stdexcept>
#include "byteseq.hpp"

CSequentialByteSequence::CSequentialByteSequence(std::initializer_list<IByteSequence*> seqs) : m_Size(0), m_NumSeqs(0) {
    m_Sequences = new seq_ent[seqs.size()];
    for(IByteSequence *seq : seqs) {
        m_Sequences[m_NumSeqs++] = seq_ent(m_Size, seq);
        m_Size += seq->size();
    }
}

CSequentialByteSequence::CSequentialByteSequence(CSequentialByteSequence &seq) : m_Size(seq.m_Size), m_NumSeqs(seq.m_NumSeqs), m_Sequences(seq.m_Sequences) {
    seq.m_Sequences = nullptr;
}

CSequentialByteSequence::~CSequentialByteSequence() {
    delete[] m_Sequences;
    m_Sequences = nullptr;
}

bool CSequentialByteSequence::set_anchor(SAnchor anchor) {
    for(int i = 0; i < m_NumSeqs; i++) {
        const seq_ent& ent = m_Sequences[i];
        if(!ent.seq->set_anchor(anchor + ent.off)) return false;
    }
    return true;
}

int CSequentialByteSequence::compare(const IByteSequence &seq, size_t this_off, size_t seq_off, size_t size) const {
    while(size > 0) {
        const seq_ent& seq_ent = get_sequence(this_off);

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
        const seq_ent& seq_ent = get_sequence(off);

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
        const seq_ent& seq_ent = get_sequence(off);

        size_t sz = std::min(seq_ent.seq->size(), size);
        seq_ent.seq->get_data(buf, off - seq_ent.off, sz);

        buf += sz;
        off += sz;
        size -= sz;
    }
}

const CSequentialByteSequence::seq_ent& CSequentialByteSequence::get_sequence(size_t off) const {
    int s = 0, e = m_NumSeqs;
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

CHexSequence::CHexSequence(const char *hexstr) {
    size_t len = 0;
    for(const char *p = hexstr; *p; p++) {
        if(!isspace(*p)) len++;
    }
    if(len % 2 != 0) throw std::invalid_argument("Invalid hex string!");

    m_Size = len / 2;
    m_Data = new uint8_t[m_Size];

    init_hex_lut();
    size_t idx = 0;
    for(const char *p = hexstr; *p; p++) {
        if(isspace(*p)) continue;
        m_Data[idx++] = (hex_lut[*p] << 4) | hex_lut[*(p+1)];
        p++;
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