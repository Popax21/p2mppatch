#include <tier0/dbg.h>
#include <sstream>
#include "patch.hpp"

CPatch::CPatch(SAnchor target, IByteSequence& orig_seq, IByteSequence& new_seq) : m_PatchTarget(target),m_IsApplied(false), m_PatchSize(orig_seq.size()) {
    if(orig_seq.size() != new_seq.size()) throw std::invalid_argument("Original and new sequences must be of the same size [" + std::to_string(orig_seq.size()) + " != " + std::to_string(new_seq.size()) + "]");

    //Convert sequences to arrays
    if(!orig_seq.set_anchor(target)) throw std::runtime_error("Couldn't anchor original sequence");
    orig_seq.get_data(m_OrigSeq = new uint8_t[m_PatchSize], 0, m_PatchSize);

    if(!new_seq.set_anchor(target)) throw std::runtime_error("Couldn't anchor new sequence");
    new_seq.get_data(m_NewSeq = new uint8_t[m_PatchSize], 0, m_PatchSize);
}

CPatch::CPatch(CPatch& patch) : m_PatchTarget(patch.m_PatchTarget), m_OrigSeq(patch.m_OrigSeq), m_NewSeq(patch.m_NewSeq), m_IsApplied(patch.m_IsApplied) {
    patch.m_IsApplied = false;
    patch.m_OrigSeq = nullptr;
    patch.m_NewSeq = nullptr;
}

CPatch::~CPatch() {
    if(m_IsApplied) revert();

    delete[] m_OrigSeq;
    delete[] m_NewSeq;
    m_OrigSeq = m_NewSeq = nullptr;
}

void CPatch::apply() {
    if(m_IsApplied) return;

    //Check if the original sequence matches the target
    if(memcmp(m_PatchTarget.get_addr(), m_OrigSeq, m_PatchSize) != 0) {
        DevWarning("Mismatching patch target / original sequence: ");
        for(size_t i = 0; i < m_PatchSize; i++) DevWarning("%02x", ((uint8_t*) m_PatchTarget.get_addr())[i]);
        DevWarning(" != ");
        for(size_t i = 0; i < m_PatchSize; i++) DevWarning("%02x", m_OrigSeq[i]);
        DevWarning("\n");

        std::stringstream sstream;
        sstream << "Original sequence of patch '" << m_PatchTarget.module->name() << "':" << std::hex << m_PatchTarget.offset << " [" << m_PatchTarget.get_addr() << "] doesn't match target [" << m_PatchSize << " bytes]";
        throw std::runtime_error(sstream.str());
    }

    //Ooverwrite the target data
    memcpy(m_PatchTarget.get_addr(), m_NewSeq, m_PatchSize);

    DevMsg("Patched %d bytes in module '%s':%lx [%p]\n", m_PatchSize, m_PatchTarget.module->name(), m_PatchTarget.offset, m_PatchTarget.get_addr());
    m_IsApplied = true;
}

void CPatch::revert() {
    if(!m_IsApplied) return;

    //Re-apply the original sequence
    memcpy(m_PatchTarget.get_addr(), m_OrigSeq, m_PatchSize);

    DevMsg("Reverted patch of %d bytes in module '%s':%lx [%p]\n", m_PatchSize, m_PatchTarget.module->name(), m_PatchTarget.offset, m_PatchTarget.get_addr());
    m_IsApplied = false;
}