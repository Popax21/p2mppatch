#include <assert.h>
#include <tier0/dbg.h>
#include <vector>
#include "detour.hpp"

const CDetour::SArgument CDetour::SArgument::reg_eax(CDetour::SArgument::EType::REG, 0);
const CDetour::SArgument CDetour::SArgument::reg_ebx(CDetour::SArgument::EType::REG, 3);
const CDetour::SArgument CDetour::SArgument::reg_ecx(CDetour::SArgument::EType::REG, 1);
const CDetour::SArgument CDetour::SArgument::reg_edx(CDetour::SArgument::EType::REG, 2);
const CDetour::SArgument CDetour::SArgument::reg_esi(CDetour::SArgument::EType::REG, 6);
const CDetour::SArgument CDetour::SArgument::reg_edi(CDetour::SArgument::EType::REG, 7);
const CDetour::SArgument CDetour::SArgument::reg_eip(CDetour::SArgument::EType::REG, -1);

bool CDetour::apply_anchor(SAnchor anchor) {
    //Build the scratchpad sequence
    CSequentialByteSequence scratch_seq;

    // - optionally copy the original asm
    if(m_DetourCopyOrigAsm) scratch_seq.emplace_sequence<CArraySequence>((const uint8_t*) anchor.get_addr(), m_DetourSize);

    std::vector<uint8_t> prefix_seq;
    unsigned int esp_off = 0;

    // - push return address
    void *ret_addr = (anchor + m_DetourSize).get_addr();
    prefix_seq.push_back(0x68);
    prefix_seq.insert(prefix_seq.end(), (uint8_t*) &ret_addr, ((uint8_t*) &ret_addr) + 4);
    esp_off += 0x4;

    // - save registers
    prefix_seq.push_back(0x60); //pusha
    esp_off += 0x20;

    // - push arguments
    for(int i = m_DetourArgs.size()-1; i >= 0; i--) {
        const SArgument& arg = m_DetourArgs[i];
        switch(arg.type) {
            case SArgument::EType::REG: {
                //lea eax, [esp + <offset to value in pusha block>]
                unsigned int off = (esp_off-4) - arg.val * 4 - 4;
                prefix_seq.insert(prefix_seq.end(), { 0x8d, 0x84, 0x24 });
                prefix_seq.insert(prefix_seq.end(), (uint8_t*) &off, ((uint8_t*) &off) + 4);
            } break;
            case SArgument::EType::LOCAL_VAR: {
                //lea eax, [ebp + <offset>]
                prefix_seq.insert(prefix_seq.end(), { 0x8d, 0x85 });
                prefix_seq.insert(prefix_seq.end(), (uint8_t*) &arg.val, ((uint8_t*) &arg.val) + 4);
            } break;
            case SArgument::EType::STACK_VAR: {
                //lea eax, [esp + <esp offset> + <offset>]
                unsigned int off = esp_off + arg.val;
                prefix_seq.insert(prefix_seq.end(), { 0x8d, 0x84, 0x24 });
                prefix_seq.insert(prefix_seq.end(), (uint8_t*) &off, ((uint8_t*) &off) + 4);
            } break;
        }

        //push eax
        prefix_seq.push_back(0x50);
        esp_off += 4;
    }

    scratch_seq.emplace_sequence<CVectorSequence>(prefix_seq);

    // - call the detour function
    scratch_seq.add_sequence(new SEQ_CALL(SAnchor(m_DetourFunc)));

    // - pop arguments and return back to the detoured code
    std::vector<uint8_t> suffix_seq;
    suffix_seq.insert(suffix_seq.end(), { 0x83, 0xc4, (uint8_t) (0x04 * m_DetourArgs.size()) }); //add esp, <4 * numArgs>
    suffix_seq.push_back(0x61); //popa
    suffix_seq.push_back(0xc3); //add esp, 4

    scratch_seq.emplace_sequence<CVectorSequence>(suffix_seq);

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