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

    SAnchor() : module(nullptr), offset(0), debug_sym(nullptr), debug_off(0) {}
    SAnchor(void *ptr, const char *debug_sym = nullptr, ssize_t debug_off = 0) : module(nullptr), offset((size_t) ptr), debug_sym(debug_sym), debug_off(debug_off) {}
    SAnchor(const CModule *module, size_t offset, const char *debug_sym = nullptr, ssize_t debug_off = 0) : module(module), offset(offset), debug_sym(debug_sym), debug_off(debug_off) {}

    void *get_addr() const;
    std::string debug_str() const;

    inline void mark_symbol(const char *sym, ssize_t sym_off = 0) {
        debug_sym = sym;
        debug_off = sym_off;
    }

    inline SAnchor operator +(size_t off) const { return SAnchor(module, offset + off, debug_sym, debug_off + off); }
    inline SAnchor operator -(size_t off) const { return SAnchor(module, offset - off, debug_sym, debug_off - off); }
    inline size_t operator -(SAnchor anchor) const { return (uintptr_t) get_addr() - (uintptr_t) anchor.get_addr(); }

    explicit inline operator bool() const { return module != nullptr || offset != 0; }

    inline bool operator ==(const SAnchor &o) const { return module == o.module && offset == o.offset; }
    inline bool operator !=(const SAnchor &o) const { return !(*this == o); }
 
    inline bool operator ==(void *o) const { return get_addr() == o; }
    inline bool operator !=(void *o) const { return get_addr() != o; }
};

inline std::ostream& operator <<(std::ostream& stream, SAnchor anchor) {
    return stream << anchor.debug_str();
}

#endif