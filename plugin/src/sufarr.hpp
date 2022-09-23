#ifndef H_P2MPPATCH_SUFARR
#define H_P2MPPATCH_SUFARR

#include "byteseq.hpp"

class CSuffixArray {
    public:
        CSuffixArray(const IByteSequence& seq, size_t max_needle_size);
        CSuffixArray(CSuffixArray& arr);
        ~CSuffixArray();

        inline const IByteSequence& get_sequence() const { return m_Sequence; }
        inline size_t get_max_needle_size() const { return m_MaxNeedleSize; }
        inline size_t get_mem_usage() const { return m_Sequence.size() * sizeof(size_t); }

        bool find_needle(const IByteSequence& needle, size_t *off) const;

        size_t *m_SufOffsets;
    private:
        void build(size_t max_needle_size);

        void sort_suffixes(size_t num_names);
        inline bool compare_suffix(size_t suf_a, size_t suf_b) const;

        const IByteSequence& m_Sequence;

        size_t m_MaxNeedleSize;
        std::unique_ptr<size_t[]> m_LexNames, m_TmpBuildArr, m_BucketIdxs;
};

#endif