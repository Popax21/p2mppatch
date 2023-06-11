#ifndef H_P2MPPATCH_PATCH
#define H_P2MPPATCH_PATCH

#include <memory>
#include "module.hpp"

class CMPPatchPlugin;

class CPatch {
    public:
        CPatch(SAnchor target, IByteSequence *orig_seq, IByteSequence *new_seq);
        CPatch(CPatch& patch);
        CPatch(const CPatch& patch) = delete;
        virtual ~CPatch();

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