#include <tier0/dbg.h>
#include <tier0/valve_minmax_off.h>

#include "plugin.hpp"
#include "hook.hpp"

void CHookTracker::prepare(CMPPatchPlugin& plugin) {
    if(m_DetourPatchTracker) return;

    int detour_size = m_HookedFuncStartOrig.size();

    //Setup the original trampoline
    //It contains the initial instructions we replaced when detouring the function, and stitches them back to the rest of the function body
    if(m_FirstTrampoline == m_OrigTrampoline.anchor()) m_FirstTrampoline = nullptr;

    CSequentialByteSequence orig_detour_seq;
    orig_detour_seq.emplace_sequence<CBufferSequence>((const uint8_t*) m_HookedFunc.get_addr(), detour_size);
    orig_detour_seq.add_sequence(new SEQ_JMP(m_HookedFunc + detour_size));
    m_OrigTrampoline = plugin.scratchpad().alloc_seq(orig_detour_seq);

    if(!m_FirstTrampoline) m_FirstTrampoline = m_OrigTrampoline.anchor().get_addr();

    //Setup the detour trampoline and register the detour patch
    void *trampoline_link_ptr = &m_FirstTrampoline;
    CScratchDetourSeq *scratch_detour = new SEQ_SCRATCH_DETOUR(plugin, detour_size, new SEQ_HEX("ff 25 $4", &trampoline_link_ptr)); //jmp dword [m_FirstTrampoline]
    plugin.register_patch(m_DetourPatchTracker.create_tracked_patch<CSeqPatch>(m_HookedFunc, new SEQ_WRAP(m_HookedFuncStartOrig), scratch_detour));
    
    DevMsg("Prepared hook tracker for function %s: detour trampoline at %s, orig trampoline at %s\n", m_HookedFunc.debug_str().c_str(), scratch_detour->scratch_entry().anchor().debug_str().c_str(), m_OrigTrampoline.anchor().debug_str().c_str());
}

void CHookTracker::add_hook(CFuncHook *hook) {
    assert(m_DetourPatchTracker && m_OrigTrampoline);

    //Add into the hook chain
    auto[it, success] = m_Hooks.insert(hook);
    assert(success);

    DevMsg("Added hook function %p into hook chain for function %s: ", hook->m_HookFunc, m_HookedFunc.debug_str().c_str());

    //Link up trampolines
    if(it == m_Hooks.begin()) {
        m_FirstTrampoline = hook->m_ScratchTrampoline.anchor().get_addr();
        DevMsg("<callsite>");
    } else {
        CFuncHook *prev_hook = *std::prev(it);
        prev_hook->m_NextTrampoline = hook->m_ScratchTrampoline.anchor().get_addr();
        DevMsg("%p [%d]", prev_hook->m_HookFunc, prev_hook->m_HookOrder);
    }

    DevMsg(" -> %p [%d] -> ", hook->m_HookFunc, hook->m_HookOrder);

    if(std::next(it) == m_Hooks.end()) {
        hook->m_NextTrampoline = m_OrigTrampoline.anchor().get_addr();
        DevMsg("<orig>\n");
    } else {
        CFuncHook *next_hook = *std::next(it);
        hook->m_NextTrampoline = next_hook->m_ScratchTrampoline.anchor().get_addr();
        DevMsg("%p [%d]\n", next_hook->m_HookFunc, next_hook->m_HookOrder);
    }
}

void CHookTracker::remove_hook(CFuncHook *hook) {
    //Remove from the hook chain
    auto it = m_Hooks.find(hook);
    assert(it != m_Hooks.end());
    it = m_Hooks.erase(it);

    DevMsg("Removed hook function %p from hook chain for function %s: ", hook->m_HookFunc, m_HookedFunc.debug_str().c_str());

    //Link up trampolines
    if(it == m_Hooks.begin()) {
        m_FirstTrampoline = hook->m_NextTrampoline;
        DevMsg("<callsite>");
    } else {
        CFuncHook *prev_hook = *std::prev(it);
        prev_hook->m_NextTrampoline = hook->m_NextTrampoline;
        DevMsg("%p [%d]", prev_hook->m_HookFunc, prev_hook->m_HookOrder);
    }

    DevMsg(" -> { %p [%d] } -> ", hook->m_HookFunc, hook->m_HookOrder);

    if(it == m_Hooks.end()) {
        DevMsg("<orig>\n");
    } else {
        DevMsg("%p [%d]\n", (*it)->m_HookFunc, (*it)->m_HookOrder);
    }
}

CFuncHook::CFuncHook(CScratchPad& scratch, CHookTracker& tracker, void *hook_func, int prio) : m_HookTracker(tracker), m_HookFunc(hook_func), m_HookOrder(prio), m_HookApplied(false), m_NextTrampoline(nullptr) {
    //Build the scratchpad trampoline for the hook
    CSequentialByteSequence trampoline_seq;
    void *trampoline_link_ptr = &m_NextTrampoline;
    trampoline_seq.add_sequence(new SEQ_HEX("a1 $4", &trampoline_link_ptr)); //mov eax, dword [m_NextTrampoline]
    trampoline_seq.add_sequence(new SEQ_JMP(hook_func));

    m_ScratchTrampoline = scratch.alloc_seq(trampoline_seq);
    DevMsg("Allocated hook trampoline at %s for hook function %p with order %d, target function: %s\n", m_ScratchTrampoline.anchor().debug_str().c_str(), hook_func, prio, tracker.hooked_func().debug_str().c_str());
}