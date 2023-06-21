#ifndef H_P2MPPATCH_HOOK
#define H_P2MPPATCH_HOOK

#include <assert.h>
#include <set>
#include <memory>
#include "module.hpp"
#include "patch.hpp"
#include "scratchpad.hpp"
#include "detour.hpp"

class CFuncHook;

class CHookTracker {
    public:
        CHookTracker(SAnchor hooked_func, IByteSequence& func_start_orig) : m_HookedFunc(hooked_func), m_FirstTrampoline(nullptr), m_HookedFuncStartOrig(func_start_orig) {}
        CHookTracker(CHookTracker&& tracker) = delete;
        CHookTracker(const CHookTracker& tracker) = delete;
        ~CHookTracker() { assert(m_Hooks.size() <= 0); }

        inline SAnchor hooked_func() const { return m_HookedFunc; }
        inline int num_hooks() const { return m_Hooks.size(); }

    private:
        SAnchor m_HookedFunc;
        IByteSequence& m_HookedFuncStartOrig;
        std::set<CFuncHook*> m_Hooks;

        CPatchTracker m_DetourPatchTracker;
        void *m_FirstTrampoline;

        CScratchPad::SSeqEntry m_OrigTrampoline;

        void prepare(CMPPatchPlugin& plugin);
        void add_hook(CFuncHook *hook);
        void remove_hook(CFuncHook *hook);

    friend class CFuncHook;
};

class CHookTrackerFact : IModuleFact<std::unique_ptr<CHookTracker>> {
    public:
        CHookTrackerFact(IModuleFact<SAnchor>& func, IByteSequence& func_start_orig) : m_Name("hook_tracker[" + std::string(func.fact_name()) + "]"), m_FuncFact(func), m_FuncStartOrig(func_start_orig) {}
        CHookTrackerFact(const CHookTrackerFact& fact) = delete;

        inline CHookTracker& get(CModule& module) { return *IModuleFact<std::unique_ptr<CHookTracker>>::get(module); }

    private:
        std::string m_Name;
        IModuleFact<SAnchor>& m_FuncFact;
        IByteSequence& m_FuncStartOrig;

        virtual const char *fact_name() const override { return m_Name.c_str(); }

        virtual std::unique_ptr<CHookTracker> determine_value(CModule& module) override {
            return std::make_unique<CHookTracker>(m_FuncFact.get(module), m_FuncStartOrig);
        }
};

class CFuncHook : public IPatch {
    public:
        CFuncHook(CScratchPad& scratch, CHookTracker& tracker, void *hook_func, int order);
        ~CFuncHook() { revert(); }

        inline CHookTracker& hook_tracker() const { return m_HookTracker; }
        inline int hook_order() const { return m_HookOrder; }

        virtual void on_register(CMPPatchPlugin& plugin) override {
            m_HookTracker.prepare(plugin);
        }

        virtual void apply() override {
            if(!m_HookApplied) m_HookTracker.add_hook(this);
            m_HookApplied = true;
        }

        virtual void revert() override {
            if(m_HookApplied) m_HookTracker.remove_hook(this);
            m_HookApplied = false;
        }

        inline bool operator <(const CFuncHook& other) const { return m_HookOrder < other.m_HookOrder; }
        inline bool operator >(const CFuncHook& other) const { return m_HookOrder > other.m_HookOrder; }

    private:
        CHookTracker& m_HookTracker;
        void *m_HookFunc;
        int m_HookOrder;
        bool m_HookApplied;

        CScratchPad::SSeqEntry m_ScratchTrampoline;
        void *m_NextTrampoline;

    friend class CHookTracker;
};

#define HOOK_FUNC __attribute__((force_align_arg_pointer)) __attribute__((regparm(1)))
#define HOOK_ORIG __attribute__((cdecl))

#endif