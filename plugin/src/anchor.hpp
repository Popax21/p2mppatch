#ifndef H_P2MPPATCH_ANCHOR
#define H_P2MPPATCH_ANCHOR

#include <stdlib.h>
#include <stdint.h>

class CModule;

struct SAnchor {
    const CModule *module;
    size_t offset;

    SAnchor() {}
    SAnchor(const CModule *module, size_t offset) : module(module), offset(offset) {}

    void *get_addr() const;

    SAnchor operator +(size_t off) const { return SAnchor(module, offset + off); }
    SAnchor operator -(size_t off) const { return SAnchor(module, offset - off); }

    size_t operator -(SAnchor anchor) const;
};

#endif