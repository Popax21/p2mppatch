#include <assert.h>
#include <tier0/dbg.h>
#include <vector>
#include "detour.hpp"

const CDetour::SArgument CDetour::SArgument::reg_eax(true, 7 - 0);
const CDetour::SArgument CDetour::SArgument::reg_ebx(true, 7 - 3);
const CDetour::SArgument CDetour::SArgument::reg_ecx(true, 7 - 2);
const CDetour::SArgument CDetour::SArgument::reg_edx(true, 7 - 1);
const CDetour::SArgument CDetour::SArgument::reg_esi(true, 7 - 6);
const CDetour::SArgument CDetour::SArgument::reg_edi(true, 7 - 7);

bool CDetour::apply_anchor(SAnchor anchor) {
    //Build the scratchpad sequence
    CSequentialByteSequence scratch_seq;

    // - push arguments
    std::vector<uint8_t> prefix_seq;

    prefix_seq.push_back(0x60); //pusha
    unsigned int pusha_esp_off = 0;

    for(int i = m_DetourArgs.size()-1; i >= 0; i--) {
        const SArgument& arg = m_DetourArgs[i];
        if(arg.is_reg) {
            //lea eax, [esp + <offset to value in pusha block>]
            unsigned int off = pusha_esp_off + arg.val * 4;
            prefix_seq.insert(prefix_seq.end(), { 0x8d, 0x84, 0x24 });
            prefix_seq.insert(prefix_seq.end(), (uint8_t*) &off, ((uint8_t*) &off) + 4);
        } else {
            //lea eax, [ebp + <offset>]
            prefix_seq.insert(prefix_seq.end(), { 0x8d, 0x85 });
            prefix_seq.insert(prefix_seq.end(), (uint8_t*) &arg.val, ((uint8_t*) &arg.val) + 4);
        }
        //push eax
        prefix_seq.push_back(0x50);
        pusha_esp_off += 4;
    }

    scratch_seq.emplace_sequence<CVectorSequence>(prefix_seq);

    // - call the detour function
    scratch_seq.add_sequence(new SEQ_CALL(SAnchor(m_DetourFunc)));

    // - pop arguments
    std::vector<uint8_t> suffix_seq;
    suffix_seq.insert(suffix_seq.end(), { 0x83, 0xc4, (uint8_t) (0x04 * m_DetourArgs.size()) }); //add esp, <4 * numArgs>
    suffix_seq.push_back(0x61); //popa
    scratch_seq.emplace_sequence<CVectorSequence>(suffix_seq);

    // - optionally copy the original asm
    if(m_DetourCopyOrigAsm) scratch_seq.emplace_sequence<CArraySequence>((const uint8_t*) anchor.get_addr(), m_DetourSize);

    // - jump back to the detoured code
    scratch_seq.add_sequence(new SEQ_JMP(anchor + m_DetourSize));

    //Allocate the sequence on the scratchpad
    m_ScratchPadEntry = m_ScratchPad.alloc_seq(scratch_seq);

    //Build the jump to the detour
    if(m_DetourSize > MIN_SIZE) {
        auto jump_seq = std::unique_ptr<CSequentialByteSequence>(new CSequentialByteSequence());
        jump_seq->add_sequence(new SEQ_JMP(m_ScratchPadEntry.anchor()));
        jump_seq->emplace_sequence<CFillSequence>(m_DetourSize - MIN_SIZE, 0x90); //Fill with NOPs
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