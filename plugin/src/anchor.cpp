#include "module.hpp"
#include "anchor.hpp"

void *SAnchor::get_addr() const { return (uint8_t*) (module ? module->base_addr() : nullptr) + offset; }
size_t SAnchor::operator -(SAnchor anchor) const { return (uintptr_t) anchor.get_addr() - (uintptr_t) get_addr(); }