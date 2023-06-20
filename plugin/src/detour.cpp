#include <assert.h>
#include <vector>
#include "detour.hpp"

SAnchor CScratchDetour::update_scratch_seq(IByteSequence& seq) {
    //Allocate the sequence on the scratchpad
    m_ScratchPadEntry = m_ScratchPad.alloc_seq(seq);

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

    if(m_DetourAnchor && !m_DetourJumpSeq->apply_anchor(m_DetourAnchor)) throw std::runtime_error("Failed to re-anchor scratchpad jump after scratch sequence update");

    return m_ScratchPadEntry.anchor();
}

const CFuncDetour::SArgument CFuncDetour::SArgument::reg_eax(CFuncDetour::SArgument::EType::REG, 0);
const CFuncDetour::SArgument CFuncDetour::SArgument::reg_ebx(CFuncDetour::SArgument::EType::REG, 3);
const CFuncDetour::SArgument CFuncDetour::SArgument::reg_ecx(CFuncDetour::SArgument::EType::REG, 1);
const CFuncDetour::SArgument CFuncDetour::SArgument::reg_edx(CFuncDetour::SArgument::EType::REG, 2);
const CFuncDetour::SArgument CFuncDetour::SArgument::reg_esi(CFuncDetour::SArgument::EType::REG, 6);
const CFuncDetour::SArgument CFuncDetour::SArgument::reg_edi(CFuncDetour::SArgument::EType::REG, 7);
const CFuncDetour::SArgument CFuncDetour::SArgument::reg_eip(CFuncDetour::SArgument::EType::REG, -1);

bool CFuncDetour::apply_anchor(SAnchor anchor) {
    //Build the scratchpad sequence
    CSequentialByteSequence scratch_seq;

    bool has_eip_arg = false;
    for(const SArgument arg : m_DetourArgs) has_eip_arg |= arg == SArgument::reg_eip;

    // - append user prefix sequences
    for(auto it = m_PrefixSeqs.rbegin(); it != m_PrefixSeqs.rend(); it++) {
        if(*it) scratch_seq.add_sequence(&**it, false);
        else scratch_seq.add_sequence(new SEQ_BUF((const uint8_t*) anchor.get_addr(), size()));
    }

    std::vector<uint8_t> prefix_seq;
    unsigned int esp_off = 0;

    // - push return address if we need it as a detour function argument
    void *ret_addr = (anchor + size()).get_addr();
    if(has_eip_arg) {
        prefix_seq.push_back(0x68); //push <return address>
        prefix_seq.insert(prefix_seq.end(), (uint8_t*) &ret_addr, ((uint8_t*) &ret_addr) + 4);
        esp_off += 0x4;
    }

    // - save registers
    unsigned int regs_off = esp_off;
    prefix_seq.push_back(0x60); //pusha
    esp_off += 0x20;

    // - push arguments
    for(int i = m_DetourArgs.size()-1; i >= 0; i--) {
        const SArgument& arg = m_DetourArgs[i];
        switch(arg.type) {
            case SArgument::EType::REG: {
                //lea eax, [esp + <offset to value in pusha block>]
                unsigned int off = (esp_off - regs_off) - arg.val * 4 - 4;
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

    // - pop arguments
    std::vector<uint8_t> suffix_seq;
    suffix_seq.insert(suffix_seq.end(), { 0x83, 0xc4, (uint8_t) (0x04 * m_DetourArgs.size()) }); //add esp, <4 * numArgs>
    suffix_seq.push_back(0x61); //popa

    scratch_seq.emplace_sequence<CVectorSequence>(suffix_seq);

    // - append user suffix sequences
    if(!has_eip_arg) {
        for(std::unique_ptr<IByteSequence>& seq : m_SuffixSeqs) {
            if(seq) scratch_seq.add_sequence(&*seq, false);
            else scratch_seq.add_sequence(new SEQ_BUF((const uint8_t*) anchor.get_addr(), size()));
        }
    } else if(m_SuffixSeqs.size() > 0) throw std::runtime_error("Can't have user suffix sequences when having EIP as a detour function argument");

    // - return back to the detoured code
    if(has_eip_arg) scratch_seq.add_sequence(new SEQ_HEX("c3")); //ret
    else scratch_seq.add_sequence(new SEQ_JMP(ret_addr));

    //Update the scratchpad sequence
    SAnchor scratch_anchor = update_scratch_seq(scratch_seq);
    DevMsg("Prepared detour from %s to function at %p via scratchpad shim at %s\n", anchor.debug_str().c_str(), m_DetourFunc, scratch_anchor.debug_str().c_str());

    //Call the original anchoring code
    return CScratchDetour::apply_anchor(anchor);
}