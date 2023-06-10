#ifndef H_P2MPPATCH_ANCHOR
#define H_P2MPPATCH_ANCHOR

#include <stdlib.h>
#include <stdint.h>
#include <sstream>

class CModule;

struct SAnchor {
    const CModule *module;
    size_t offset;

    SAnchor() {}
    SAnchor(void *ptr) : module(nullptr), offset((size_t) ptr) {}
    SAnchor(const CModule *module, size_t offset) : module(module), offset(offset) {}

    void *get_addr() const;
    std::string debug_str() const;

    SAnchor operator +(size_t off) const { return SAnchor(module, offset + off); }
    SAnchor operator -(size_t off) const { return SAnchor(module, offset - off); }

    size_t operator -(SAnchor anchor) const;
};

#endif