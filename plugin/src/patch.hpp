#ifndef H_P2MPPATCH_PATCH
#define H_P2MPPATCH_PATCH

#include <memory>
#include "module.hpp"

class CMPPatchPlugin;

class IPatch {
    public:
        virtual ~IPatch() {}

        virtual void on_register(CMPPatchPlugin& plugin) {}
        virtual void apply() = 0;
        virtual void revert() = 0;
};

class CPatchTracker {
    public:
        CPatchTracker() : m_IsAlive(false), m_PatchOwner(nullptr), m_PatchUser(nullptr) {}
        CPatchTracker(std::unique_ptr<IPatch> patch) : m_IsAlive(true), m_PatchOwner(std::move(patch)), m_PatchUser(nullptr) {}
        CPatchTracker(CPatchTracker&& tracker) : CPatchTracker() { *this = std::move(tracker); }
        CPatchTracker(const CPatchTracker& tracker) = delete;
        ~CPatchTracker() {
            if(!m_PatchUser) return;
            m_PatchUser->m_Tracker = nullptr;
            m_PatchUser = nullptr;
        }

        inline bool is_alive() const { return m_IsAlive; }
        inline std::unique_ptr<IPatch>& patch_owner() { return m_PatchOwner; }
        inline const std::unique_ptr<IPatch>& patch_owner() const { return m_PatchOwner; }
        inline IPatch *patch_user() const { return m_PatchUser; }

        std::unique_ptr<IPatch> create_tracked_patch(IPatch *patch) {
            *this = CPatchTracker(std::unique_ptr<IPatch>(patch));
            return make_patch_user();
        }
        template<typename T, typename... Args> std::unique_ptr<IPatch> create_tracked_patch(Args&&... args) { return create_tracked_patch(new T(args...)); }

        inline CPatchTracker& operator =(CPatchTracker&& tracker) {
            if(m_PatchUser) throw std::runtime_error("Can't re-assign this tracker as the old patch user is still alive");

            //Re-assign this tracker by moving over the other tracker's values
            m_IsAlive = tracker.m_IsAlive;
            m_PatchOwner = std::move(tracker.m_PatchOwner);
            if(m_PatchUser = tracker.m_PatchUser) m_PatchUser->m_Tracker = this;

            tracker.m_IsAlive = false;
            tracker.m_PatchUser = nullptr;

            return *this;
        }

        std::unique_ptr<IPatch> make_patch_user() {
            if(!m_IsAlive) throw std::runtime_error("CPatchTracker has no alive tracked patch");
            if(m_PatchUser) throw std::runtime_error("A tracked patch user has already been created");
            return std::unique_ptr<IPatch>(m_PatchUser = new CPatchUser(this));
        }

        inline explicit operator bool() const { return m_IsAlive; }

    protected:
        virtual void on_patch_user_deletion() { m_PatchOwner.reset(); }

    private:
        class CPatchUser : public IPatch {
            public:
                CPatchUser(CPatchTracker *tracker) : m_Tracker(tracker) {}
                CPatchUser(CPatchUser&& user) : m_Tracker(user.m_Tracker) {
                    if(m_Tracker) m_Tracker->m_PatchUser = this;
                    user.m_Tracker = nullptr;
                }
                CPatchUser(const CPatchUser& user) = delete;
                ~CPatchUser() {
                    if(!m_Tracker) return;
                    m_Tracker->on_patch_user_deletion();
                    m_Tracker->m_PatchUser = nullptr;
                    m_Tracker->m_IsAlive = false;
                    m_Tracker = nullptr;
                }

                virtual void on_register(CMPPatchPlugin& plugin) override {
                    if(m_Tracker && m_Tracker->m_PatchOwner) m_Tracker->m_PatchOwner->on_register(plugin);
                }
                virtual void apply() override {
                    if(m_Tracker && m_Tracker->m_PatchOwner) m_Tracker->m_PatchOwner->apply();
                }
                virtual void revert() override {
                    if(m_Tracker && m_Tracker->m_PatchOwner) m_Tracker->m_PatchOwner->revert();
                }

                CPatchTracker *m_Tracker;
        };

        bool m_IsAlive;
        std::unique_ptr<IPatch> m_PatchOwner;
        CPatchUser *m_PatchUser;
};

class CSeqPatch : public IPatch {
    public:
        CSeqPatch(SAnchor target, IByteSequence *orig_seq, IByteSequence *new_seq);
        CSeqPatch(CSeqPatch&& patch);
        CSeqPatch(const CSeqPatch& patch) = delete;
        virtual ~CSeqPatch();

        virtual void apply();
        virtual void revert();

    protected:
        SAnchor m_PatchTarget;
        bool m_IsApplied;

        std::unique_ptr<IByteSequence> m_OrigSeq, m_NewSeq;
        size_t m_PatchSize;
        uint8_t *m_OrigData;
};

class IPatchRegistrar {
    public:
        virtual const char *name() const = 0;

        virtual void register_patches(CMPPatchPlugin& plugin) = 0;
        virtual void after_patch_status_change(CMPPatchPlugin& plugin, bool applied) {}
};

#endif