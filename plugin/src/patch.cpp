#include <tier0/dbg.h>
#include "patch.hpp"

CPatch::CPatch(SAnchor target, IByteSequence& orig_seq, IByteSequence& new_seq) : m_PatchTarget(target), m_OrigSeq(orig_seq), m_NewSeq(new_seq) {
    if(orig_seq.size() != new_seq.size()) throw std::invalid_argument("Original and new sequences must be of the same size");
}

CPatch::CPatch(CPatch& patch) : m_PatchTarget(patch.m_PatchTarget), m_OrigSeq(patch.m_OrigSeq), m_NewSeq(patch.m_NewSeq), m_IsApplied(patch.m_IsApplied) {
    patch.m_IsApplied = false;
}

CPatch::~CPatch() {
    if(m_IsApplied) revert();
}

void CPatch::apply() {
    if(m_IsApplied) throw std::runtime_error("Can't apply patch twice");

    //Check if the original sequence matches the target
    m_OrigSeq.set_anchor(m_PatchTarget);
    if(m_OrigSeq.compare((uint8_t*) m_PatchTarget.get_addr(), 0, m_OrigSeq.size()) != 0) throw new std::runtime_error("Original sequence of patch '" + std::string(m_PatchTarget.module->name()) + "':" + std::to_string(m_PatchTarget.offset) + "doesn't match target [" + std::to_string(m_OrigSeq.size()) + " bytes]");

    //Anchor the new sequence and overwrite the target data
    m_NewSeq.set_anchor(m_PatchTarget);
    m_NewSeq.get_data((uint8_t*) m_PatchTarget.get_addr(), 0, m_NewSeq.size());

    DevMsg("Patched %d bytes in module '%s' at offset 0x%llx\n", m_OrigSeq.size(), m_PatchTarget.module->name(), m_PatchTarget.offset);
    m_IsApplied = true;
}

void CPatch::revert() {
    if(!m_IsApplied) throw std::runtime_error("Can't revert patch which isn't applied");

    //Re-apply the original sequence
    m_OrigSeq.set_anchor(m_PatchTarget);
    m_OrigSeq.get_data((uint8_t*) m_PatchTarget.get_addr(), 0, m_OrigSeq.size());

    DevMsg("Reverted patch of %d bytes in module '%s' at offset 0x%llx\n", m_OrigSeq.size(), m_PatchTarget.module->name(), m_PatchTarget.offset);
    m_IsApplied = false;
}