#pragma GCC optimize("O3")

#include <assert.h>
#include <tier0/dbg.h>
#include <chrono>
#include <algorithm>
#include "sufarr.hpp"

CSuffixArray::CSuffixArray(const IByteSequence& seq, size_t max_needle_size) : m_Sequence(seq) {
    //Build the array
    build(max_needle_size);
}

CSuffixArray::CSuffixArray(CSuffixArray& arr) : m_Sequence(arr.m_Sequence), m_SufOffsets(arr.m_SufOffsets) {
    arr.m_SufOffsets = nullptr;
}

CSuffixArray::~CSuffixArray() {
    delete[] m_SufOffsets;
    m_SufOffsets = nullptr;
}

bool CSuffixArray::find_needle(const IByteSequence& needle, size_t *off) const {
    const size_t needle_sz = needle.size();
    if(needle_sz > m_MaxNeedleSize) return false;

    //Use binary search to find the suffix starting with the prefix
    size_t s = 0, e = m_Sequence.size();
    while(s < e-1) {
        size_t m = s + (e-s) / 2;
        if(needle.compare(m_Sequence, 0, m_SufOffsets[m], needle_sz) <= 0) e = m;
        else s = m;
    }

    //Check if the suffix matches the needle
    size_t suf = s;
    if(suf > 0 || needle.compare(m_Sequence, 0, m_SufOffsets[suf], needle_sz) != 0) {
        suf++;
        if(suf >= m_Sequence.size()) return false;
        if(needle.compare(m_Sequence, 0, m_SufOffsets[suf], needle_sz) != 0) return false;
    }

    //Check if the suffix is unique
    if(suf+1 < m_Sequence.size() && needle.compare(m_Sequence, 0, m_SufOffsets[suf+1], needle_sz) == 0) return false;

    *off = m_SufOffsets[suf];
    return true;
}

void CSuffixArray::build(size_t max_needle_size) {
    const size_t seq_sz = m_Sequence.size();
    m_SufOffsets = new size_t[seq_sz];

    //Allocate arrays
    m_LexNames = std::unique_ptr<size_t[]>(new size_t[seq_sz]);
    m_TmpBuildArr = std::unique_ptr<size_t[]>(new size_t[seq_sz]);
    m_BucketIdxs = std::unique_ptr<size_t[]>(new size_t[std::max((size_t) 256, seq_sz)]);
    m_BucketIdxsOff = std::unique_ptr<size_t[]>(new size_t[std::max((size_t) 256, seq_sz)]);

    //Initialize suffixes
    size_t num_names = 256;
    for(size_t i = 0; i < seq_sz; i++) {
        m_SufOffsets[i] = i;
        m_LexNames[i] = m_Sequence[i];
    }

    //Build loop
    for(m_MaxNeedleSize = 1; m_MaxNeedleSize < max_needle_size; m_MaxNeedleSize *= 2) {
        using namespace std::chrono;
        auto t1 = high_resolution_clock::now();

        //Sort suffixes by their lexicographic names
        sort_suffixes(num_names);
        auto tsort = high_resolution_clock::now();

        //Assign suffixes new lexicographic names
        num_names = 1;
        for(size_t i = 0; i < seq_sz; i++) {
            size_t suf = m_SufOffsets[i];
            if(i > 0 && compare_suffix(m_SufOffsets[i-1], suf)) num_names++;
            m_TmpBuildArr[suf] = num_names-1;
        }

        //Swap name arrays
        m_LexNames.swap(m_TmpBuildArr);

        auto t2 = high_resolution_clock::now();
        DevMsg("Finished suffix array build iteration with size=%zu/%zu, %zu total names, took %dms (sort: %dms, assignment: %dms)\n", m_MaxNeedleSize, max_needle_size, num_names,
            (int) duration_cast<milliseconds>(t2 - t1).count(), (int) duration_cast<milliseconds>(tsort - t1).count(), (int) duration_cast<milliseconds>(t2 - tsort).count()
        );

        //Check if we assigned each suffix an unique name
        if(num_names == seq_sz) break;
    }

    //Free arrays
    m_LexNames.reset();
    m_TmpBuildArr.reset();
    m_BucketIdxs.reset();
    m_BucketIdxsOff.reset();

    m_MaxNeedleSize = max_needle_size;
}

#undef assert
#define assert(x)

void CSuffixArray::sort_suffixes(size_t num_names) {
    const size_t seq_sz = m_Sequence.size();
    assert(num_names <= std::max((size_t) 256, seq_sz));

    //Count primary and offset lexicographic names
    std::fill_n(m_BucketIdxs.get(), num_names, 0);
    std::fill_n(m_BucketIdxsOff.get(), num_names, 0);
    for(size_t i = 0; i < seq_sz; i++) {
        size_t lex_name = m_LexNames[i];
        assert(lex_name < num_names);

        m_BucketIdxs[lex_name]++;
        if(i >= m_MaxNeedleSize) m_BucketIdxsOff[lex_name]++;
    }

    //Assign each bucket a start index
    size_t bucket_idx = m_MaxNeedleSize, off_bucket_idx = m_MaxNeedleSize;
    for(size_t i = 0; i < num_names; i++) {
        size_t name_cnt = m_BucketIdxs[i];
        m_BucketIdxs[i] = bucket_idx;
        bucket_idx += name_cnt;

        size_t off_name_cnt = m_BucketIdxsOff[i];
        m_BucketIdxsOff[i] = off_bucket_idx;
        off_bucket_idx += off_name_cnt;
    }
    assert(bucket_idx == seq_sz);
    assert(off_bucket_idx == seq_sz);

    //<<< Bucket sort - pass 1 >>>
    //Sort by secondary lexicographic names / sequence length
    //Sort suffixes into temporary buffer by either their lexicographic name, or their length
    for(size_t i = 0; i < seq_sz; i++) {
        if(i + m_MaxNeedleSize < seq_sz) {
            //Add to the right bucket
            size_t lex_name = m_LexNames[i + m_MaxNeedleSize];
            assert(m_BucketIdxsOff[lex_name] < (lex_name < num_names - 1 ? m_BucketIdxsOff[lex_name+1] : seq_sz));
            m_TmpBuildArr[m_BucketIdxsOff[lex_name]++] = i;
        } else {
            //Add to the right stop in front
            m_TmpBuildArr[seq_sz - i - 1] = i;
        }
    }

    //<<< Bucket sort - pass 2 >>>
    //Sort by primary lexicographic names, keeping order of previous pass
    //Sort suffixes into sorted suffix array, keeping order of temporary buffer
    for(size_t i = 0; i < seq_sz; i++) {
        size_t suf = m_TmpBuildArr[i];

        //Add to the right bucket
        size_t lex_name = m_LexNames[suf];
        assert(m_BucketIdxs[lex_name] < (lex_name < num_names - 1 ? m_BucketIdxs[lex_name+1] : seq_sz));
        m_SufOffsets[m_BucketIdxs[lex_name]++] = suf;
    }
}

inline bool CSuffixArray::compare_suffix(size_t suf_a, size_t suf_b) const {
    //Compare lexicographic names at the start of the suffixes
    if(m_LexNames[suf_a] != m_LexNames[suf_b]) return m_LexNames[suf_a] < m_LexNames[suf_b];

    //Compare lexicographic names at the current build offset
    suf_a += m_MaxNeedleSize;
    suf_b += m_MaxNeedleSize;
    if(suf_a >= m_Sequence.size() || suf_b >= m_Sequence.size()) return suf_a > suf_b;
    return m_LexNames[suf_a] < m_LexNames[suf_b];
}