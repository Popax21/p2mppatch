#ifndef H_P2MPPATCH_PATCH
#define H_P2MPPATCH_PATCH

#include "module.hpp"

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
        IByteSequence& m_OrigSeq;
        IByteSequence& m_NewSeq;
        bool m_IsApplied;
};

#endif