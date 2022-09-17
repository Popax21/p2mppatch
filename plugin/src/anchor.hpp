#ifndef H_P2MPPATCH_ANCHOR
#define H_P2MPPATCH_ANCHOR

#include <stdlib.h>
#include <stdint.h>

class CModule;

struct SAnchor {
    const CModule& module;
    size_t offset;

    SAnchor(const CModule& module, size_t offset) : module(module), offset(offset) {}

    SAnchor operator +(size_t off) const { return SAnchor(module, offset + off); }
};

#endif