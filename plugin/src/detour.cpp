#include <assert.h>
#include <tier0/dbg.h>
#include <vector>
#include "detour.hpp"

const CDetour::SArgument CDetour::SArgument::REG_EAX(true, 0);
const CDetour::SArgument CDetour::SArgument::REG_EBX(true, 3);
const CDetour::SArgument CDetour::SArgument::REG_ECX(true, 2);
const CDetour::SArgument CDetour::SArgument::REG_EDX(true, 1);
const CDetour::SArgument CDetour::SArgument::REG_ESI(true, 6);
const CDetour::SArgument CDetour::SArgument::REG_EDI(true, 7);

bool CDetour::apply_anchor(SAnchor anchor) {
    //Build the scratchpad sequence
    CSequentialByteSequence scratch_seq;

    // - push arguments
    std::vector<uint8_t> prefix_seq;
    prefix_seq.push_back(0x60); //pusha
    for(int i = 0; i < m_DetourArgs.size(); i++) {
        const SArgument& arg = m_DetourArgs[i];
        if(arg.is_reg) {
            //push <reg>
            prefix_seq.push_back(0x50 + arg.val);
        } else {
            //add ebp, <offset>
            //push ebp
            //sub ebp, <offset>
            prefix_seq.insert(prefix_seq.end(), { 0x81, 0xc4 });
            prefix_seq.insert(prefix_seq.end(), (uint8_t*) &arg.val, ((uint8_t*) &arg.val) + 4);
            prefix_seq.push_back(0x55);
            prefix_seq.insert(prefix_seq.end(), { 0x81, 0xec });
            prefix_seq.insert(prefix_seq.end(), (uint8_t*) &arg.val, ((uint8_t*) &arg.val) + 4);
        }
    }
    scratch_seq.emplace_sequence<CVectorSequence>(prefix_seq);

    scratch_seq.add_sequence(new SEQ_CALL(SAnchor(m_DetourFunc)));

    // - pop arguments
    std::vector<uint8_t> suffix_seq;
    for(int i = m_DetourArgs.size() - 1; i >= 0; i--) {
        const SArgument& arg = m_DetourArgs[i];
        if(arg.is_reg) {
            //pop <reg>
            suffix_seq.push_back(0x58 + arg.val);
        } else {
            //add esp, 4
            suffix_seq.insert(suffix_seq.end(), { 0x83, 0xc4, 0x32 });
        }
    }
    suffix_seq.push_back(0x61); //popa
    scratch_seq.emplace_sequence<CVectorSequence>(suffix_seq);

    scratch_seq.add_sequence(new SEQ_JMP(anchor + m_DetourSize));

    //Allocate the sequence on the scratchpad
    m_ScratchPadEntry = m_ScratchPad.alloc_seq(scratch_seq);

    //Build the jump to the detour
    if(m_DetourSize > MIN_SIZE) {
        auto jump_seq = std::unique_ptr<CSequentialByteSequence>(new CSequentialByteSequence());
        jump_seq->add_sequence(new SEQ_JMP(m_ScratchPadEntry.anchor()));
        jump_seq->emplace_sequence<CFillByteSequence>(m_DetourSize - MIN_SIZE, 0x90); //Fill with NOPs
        m_DetourJumpSeq = std::unique_ptr<IByteSequence>(jump_seq.release());
    } else {
        m_DetourJumpSeq = std::unique_ptr<CRefInstructionSequence>(new SEQ_JMP(m_ScratchPadEntry.anchor()));
    }
    assert(m_DetourJumpSeq->size() == m_DetourSize);

    if(!m_DetourJumpSeq->apply_anchor(anchor)) return false;
    
    std::string anchor_str = anchor.debug_str();
    DevMsg("Prepared detour from %s to function at %p\n", anchor_str.c_str(), m_DetourFunc);
    return true;
}