#include <stdexcept>
#include "byteseq.hpp"

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
    int len = strlen(hexstr);
    if(len % 2 != 0) throw std::invalid_argument("Invalid hex string!");

    m_Size = len / 2;
    m_Data = new uint8_t[m_Size];

    init_hex_lut();
    for(size_t i = 0; i < m_Size; i++) {
        m_Data[i] = (hex_lut[hexstr[2*i]] << 4) | hex_lut[hexstr[2*i+1]];
    }
}

CHexSequence::CHexSequence(const CHexSequence &seq) {
    m_Data = new uint8_t[m_Size = seq.m_Size];
    memcpy(m_Data, seq.m_Data, m_Size);
}

CHexSequence::~CHexSequence() {
    delete m_Data;
    m_Data = nullptr;
}