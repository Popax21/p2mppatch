#ifndef H_P2MPPATCH_ANCHOR
#define H_P2MPPATCH_ANCHOR

#include <stdlib.h>
#include <stdint.h>
#include <sstream>

class CModule;

struct SAnchor {
    const CModule *module;
    size_t offset;

    const char *debug_sym;
    ssize_t debug_off;

    SAnchor() : module(nullptr), debug_sym(nullptr) {}
    SAnchor(void *ptr, const char *debug_sym = nullptr, ssize_t debug_off = 0) : module(nullptr), offset((size_t) ptr), debug_sym(debug_sym), debug_off(debug_off) {}
    SAnchor(const CModule *module, size_t offset, const char *debug_sym = nullptr, ssize_t debug_off = 0) : module(module), offset(offset), debug_sym(debug_sym), debug_off(debug_off) {}

    void *get_addr() const;
    std::string debug_str() const;

    void mark_symbol(const char *sym, ssize_t sym_off = 0) {
        debug_sym = sym;
        debug_off = sym_off;
    }

    SAnchor operator +(size_t off) const { return SAnchor(module, offset + off, debug_sym, debug_off + off); }
    SAnchor operator -(size_t off) const { return SAnchor(module, offset - off, debug_sym, debug_off - off); }
    size_t operator -(SAnchor anchor) const { return (uintptr_t) get_addr() - (uintptr_t) anchor.get_addr(); }
};

#endif