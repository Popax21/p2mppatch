#ifndef H_P2MPPATCH_PATCH
#define H_P2MPPATCH_PATCH

#include "module.hpp"

class CMPPatchPlugin;

class CPatch {
    public:
        CPatch(SAnchor target, IByteSequence& orig_seq, IByteSequence& new_seq);
        CPatch(CPatch& patch);
        CPatch(const CPatch& patch) = delete;
        virtual ~CPatch();

        virtual void apply();
        virtual void revert();

    protected:
        SAnchor m_PatchTarget;
        bool m_IsApplied;

        uint8_t *m_OrigSeq, *m_NewSeq;
        size_t m_PatchSize;
};

class IPatchRegistrar {
    public:
        virtual const char *name() const = 0;
        virtual void register_patches(CMPPatchPlugin& plugin) = 0;
};

#endif