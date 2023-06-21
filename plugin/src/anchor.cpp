#include "module.hpp"
#include "anchor.hpp"

void *SAnchor::get_addr() const { return (uint8_t*) (module ? module->base_addr() : nullptr) + offset; }

std::string SAnchor::debug_str() const {
    std::ostringstream stream;
    if(debug_sym) {
        stream << debug_sym;
        if(debug_off != 0) stream << "+" << std::hex << debug_off;
        stream << " [" << get_addr() << "]";
    } else if(module) stream << module->name() << "+" << std::hex << offset << " [" << get_addr() << "]";
    else stream << get_addr();
    return stream.str();
}