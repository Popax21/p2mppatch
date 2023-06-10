#include "module.hpp"
#include "anchor.hpp"

void *SAnchor::get_addr() const { return (uint8_t*) (module ? module->base_addr() : nullptr) + offset; }

std::string SAnchor::debug_str() const {
    std::ostringstream stream;
    if(module) stream << "'" << module->name() << "':" << std::hex << offset << " [" << get_addr() << "]";
    else stream << get_addr();
    return stream.str();
}

size_t SAnchor::operator -(SAnchor anchor) const { return (uintptr_t) anchor.get_addr() - (uintptr_t) get_addr(); }