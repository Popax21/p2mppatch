#include <tier0/dbg.h>
#include <sstream>
#include "patch.hpp"

CPatch::CPatch(SAnchor target, IByteSequence *orig_seq, IByteSequence *new_seq) : m_PatchTarget(target), m_IsApplied(false), m_OrigSeq(orig_seq), m_NewSeq(new_seq), m_PatchSize(orig_seq->size()) {
    if(orig_seq->size() != new_seq->size()) {
        std::stringstream sstream;
        sstream << "Original and new sequences must be of the same size [" << orig_seq->size() << " != " << new_seq->size() << "]";
        throw std::invalid_argument(sstream.str());
    }

    m_OrigData = new uint8_t[m_PatchSize];
}

CPatch::CPatch(CPatch& patch) : m_PatchTarget(patch.m_PatchTarget), m_IsApplied(patch.m_IsApplied) {
    patch.m_IsApplied = false;
    m_OrigSeq = std::move(patch.m_OrigSeq);
    m_NewSeq = std::move(patch.m_NewSeq);
}

CPatch::~CPatch() {
    if(m_IsApplied) revert();

    if(m_OrigData) delete[] m_OrigData;
    m_OrigData = nullptr;
}

void CPatch::apply() {
    if(m_IsApplied) return;

    //Anchor sequences
    if(!m_OrigSeq->apply_anchor(m_PatchTarget)) throw std::runtime_error("Couldn't anchor original patch sequence");
    if(!m_NewSeq->apply_anchor(m_PatchTarget)) throw std::runtime_error("Couldn't anchor new patch sequence");

    //Check if the original sequence matches the target
    if(m_OrigSeq->compare((uint8_t*) m_PatchTarget.get_addr(), 0, m_PatchSize) != 0) {
        DevWarning("Mismatching patch target / original sequence: ");
        for(size_t i = 0; i < m_PatchSize; i++) DevWarning("%02x", ((uint8_t*) m_PatchTarget.get_addr())[i]);
        DevWarning(" != ");
        for(size_t i = 0; i < m_PatchSize; i++) DevWarning("%02x", (*m_OrigSeq)[i]);
        DevWarning("\n");

        std::stringstream sstream;
        sstream << "Original sequence of patch " << m_PatchTarget.debug_str() << " doesn't match target [" << m_PatchSize << " bytes]";
        throw std::runtime_error(sstream.str());
    }

    //Overwrite the target data
    uint8_t *patch_target = (uint8_t*) m_PatchTarget.get_addr();
    memcpy(m_OrigData, patch_target, m_PatchSize);
    m_NewSeq->get_data(patch_target, 0, m_PatchSize);

    std::string target_str = m_PatchTarget.debug_str();
    DevMsg("Patched %d bytes at location %s\n", m_PatchSize, target_str.c_str());
    m_IsApplied = true;
}

void CPatch::revert() {
    if(!m_IsApplied) return;

    //Re-apply the original sequence
    memcpy(m_PatchTarget.get_addr(), m_OrigData, m_PatchSize);

    std::string target_str = m_PatchTarget.debug_str();
    DevMsg("Reverted patch of %d bytes at location %s\n", m_PatchSize, target_str.c_str());
    m_IsApplied = false;
}