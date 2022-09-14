#ifndef H_P2MPPATCH_SUFTREE
#define H_P2MPPATCH_SUFTREE

#include "byteseq.hpp"

class CSuffixTree {
    public:
        CSuffixTree(const IByteSequence& seq);
        ~CSuffixTree();

        inline const IByteSequence& get_sequence() const { return m_Sequence; }

        size_t get_mem_usage() const { return m_MemUsage; }

        bool find_needle(const IByteSequence& needle, size_t *off) const;

    private:
        static const int NUM_ORDERS = 257;

        struct tree_node;
        typedef tree_node *tree_node_ref;

        //Tree construction
        void build_tree(size_t seq_size);
        void setup_suffixes(size_t seq_size, tree_node *node, size_t suf_size);

        //Node pool
        tree_node *alloc_node(size_t lab_start, size_t lab_end, int order);
        void free_node(tree_node *node);

        tree_node *new_leaf(size_t lab_start);
        tree_node *new_bifurc(size_t lab_start, size_t lab_end, tree_node *node_a, tree_node *node_b);
        tree_node *add_node_child(tree_node_ref *node, tree_node *child);

        //Miscellaneous
        const IByteSequence& m_Sequence;
        size_t m_LeafSuffixEnd;
        tree_node *m_RootNode;

        //Node pool
        struct npool_page;
        union npool_ent;
        npool_page *m_NodePoolPages[NUM_ORDERS];
        npool_ent *m_NodePoolFreeHead[NUM_ORDERS];
        size_t m_MemUsage;
};

#endif