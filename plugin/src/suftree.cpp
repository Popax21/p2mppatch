#include <assert.h>
#include <limits>
#include <algorithm>
#include <tier0/dbg.h>
#include "suftree.hpp"

const int NPOOL_PAGE_SIZE = 1024;

struct CSuffixTree::tree_node {
    static const size_t LABEL_START_ROOT = std::numeric_limits<size_t>::max(), LABEL_END_LEAF = LABEL_START_ROOT;

    struct cslot {
        uint8_t symbol;
        tree_node_ref node;

        cslot(uint8_t symbol, tree_node *node) : symbol(symbol), node((tree_node_ref) node) {}
    } __attribute__((packed));

    size_t lab_start, lab_end;
    uint8_t order_m1;

    union {
        tree_node_ref *suf_link;
        size_t suf_offset;
    };

    cslot children[];

    tree_node(size_t lab_start, size_t lab_end, uint8_t order) : lab_start(lab_start), lab_end(lab_end), order_m1((uint8_t) (order-1)), suf_link(nullptr) {}

    inline bool is_root() const { return lab_start == LABEL_START_ROOT; }
    inline bool is_leaf() const { return lab_end == LABEL_END_LEAF; }
    inline size_t label_end(const CSuffixTree& tree) const { return lab_end == LABEL_END_LEAF ? tree.m_LeafSuffixEnd : lab_end; }
    inline size_t label_size(const CSuffixTree& tree) const { return label_end(tree) - lab_start; }
    inline int order() const { return is_leaf() ? 0 : (order_m1 + 1); }

    tree_node_ref *operator [](uint8_t sym) {
        return (tree_node_ref*) (*(const tree_node*) this)[sym];
    }

    const tree_node_ref *operator [](uint8_t sym) const {
        if(is_leaf()) return nullptr;

        int s = 0, e = order_m1+1;
        while(s < e-1) {
            int m = s + (e-s) / 2;
            if(sym < children[m].symbol) e = m;
            else s = m;
        }

        if(children[s].symbol != sym) return nullptr;
        return &children[s].node;
    }
} __attribute__((packed));

struct CSuffixTree::npool_page {
    npool_page *next_page;
    uint8_t nodes[];
};

union CSuffixTree::npool_ent {
    tree_node node;
    npool_ent *next_free;
};

CSuffixTree::CSuffixTree(const IByteSequence& seq) : m_Sequence(seq), m_LeafSuffixEnd(0), m_MemUsage(0) {
    for(int i = 0; i < NUM_ORDERS; i++) {
        m_NodePoolPages[i] = nullptr;
        m_NodePoolFreeHead[i] = nullptr;
    }

    //Build the tree
    size_t size = seq.size();
    build_tree(size);
    setup_suffixes(size, m_RootNode, 0);
}

CSuffixTree::~CSuffixTree() {
    //Free the node pool
    for(int i = 0; i < NUM_ORDERS; i++) {
        //Free pages
        for(npool_page *p = m_NodePoolPages[i], *np = p ? p->next_page : nullptr; p; p = np, np = p ? p->next_page : nullptr) {
            delete p;
        }

        m_NodePoolPages[i] = nullptr;
        m_NodePoolFreeHead[i] = nullptr;
    }
    m_MemUsage = 0;
}

bool CSuffixTree::find_needle(const IByteSequence& needle, size_t *off) const {
    size_t seq_off = 0, seq_size = needle.size();

    const tree_node *node = m_RootNode;
    while(seq_off < seq_size) {
        //Descend to the next child
        const tree_node_ref *child_ref = (*node)[needle[seq_off]];
        if(!child_ref) return false;
        node = *child_ref;

        //Compare its label
        size_t lab_size = node->label_size(*this);
        if(!needle.compare(m_Sequence, seq_off, node->lab_start, std::min(needle.size() - seq_off, lab_size))) return false;
        seq_off += lab_size;
    }

    //For the match to be unique, we must have ended up at a leaf node
    if(!node->is_leaf()) return false;

    *off = node->suf_offset;
    return true;
}

//Ukkonen's algorithm
inline void CSuffixTree::build_tree(size_t seq_size) {
    //Create root node
    m_RootNode = new_leaf(tree_node::LABEL_START_ROOT);

    //Iterative expansion
    tree_node *actv_node = m_RootNode;
    tree_node_ref *actv_node_ref = &m_RootNode;
    size_t actv_len = 0, actv_off = 0;

    size_t rem_suffixes = 0;
    for(size_t cur_off = 0; cur_off < seq_size; cur_off++) {
        uint8_t cur_sym = m_Sequence[cur_off];

        //Extend leafs
        m_LeafSuffixEnd = cur_off+1;

        //Add remaining suffixes
        tree_node *suf_link_node = nullptr;

        rem_suffixes++;
        while(rem_suffixes > 0) {
            //Set the active symbol to the current one when branching
            tree_node_ref *actv_edge_node_ref = nullptr;
            while(true) {
                uint8_t actv_sym;
                if(actv_len <= 0) {
                    actv_off = cur_off;
                    actv_sym = cur_sym;
                } else actv_sym = m_Sequence[actv_off];

                //Get the child along the active edge
                actv_edge_node_ref = (*actv_node)[actv_sym];
                if(!actv_edge_node_ref) break;

                //Check if we can go down the edge
                size_t lab_sz = (*actv_edge_node_ref)->label_size(*this);
                if(actv_len < lab_sz) break;

                actv_node = *(actv_node_ref = actv_edge_node_ref);
                actv_len -= lab_sz;
                actv_off += lab_sz;
            }

            if(actv_edge_node_ref) {
                tree_node *actv_edge_node = *actv_edge_node_ref;

                //Check if the edge already matches the suffix
                if(m_Sequence[actv_edge_node->lab_start + actv_len] == cur_sym) {
                    //Set suffix link
                    if(suf_link_node) suf_link_node->suf_link = actv_node_ref;
                    suf_link_node = actv_node;

                    actv_len++;
                    break;
                }

                //Bifurcate the edge
                size_t bifurc_start = actv_edge_node->lab_start;
                actv_edge_node->lab_start += actv_len;
                tree_node *bifurc_node = new_bifurc(bifurc_start, bifurc_start + actv_len, actv_edge_node, new_leaf(cur_off));
                *actv_edge_node_ref = bifurc_node;

                //Set suffix link
                if(suf_link_node) suf_link_node->suf_link = actv_edge_node_ref;
                suf_link_node = bifurc_node;
            } else {
                //There's no existing node with this suffix in the tree
                //Add a new leaf node
                actv_node = add_node_child(actv_node_ref, new_leaf(cur_off));

                //Set suffix link
                if(suf_link_node) suf_link_node->suf_link = actv_node_ref;
                suf_link_node = actv_node;
            }

            rem_suffixes--;
            if(actv_node == m_RootNode && actv_len > 0) {
                actv_len--;
                actv_off = cur_off - rem_suffixes + 1;
            } else if(actv_node != m_RootNode) {
                //Follow suffix link
                actv_node = *(actv_node_ref = actv_node->suf_link);
            }
        }
    }
}

inline void CSuffixTree::setup_suffixes(size_t seq_size, tree_node *node, size_t suf_size) {
    if(!node->is_root()) suf_size += node->label_size(*this);

    if(node->is_leaf()) {
        node->suf_offset = seq_size - suf_size;
    } else {
        for(int i = 0, o = node->order(); i < o; i++) {
            setup_suffixes(seq_size, (*node).children[i].node, suf_size);
        }
    }
}

inline CSuffixTree::tree_node *CSuffixTree::alloc_node(size_t lab_start, size_t lab_end, int order) {
    //Check free list
    if(!m_NodePoolFreeHead[order]) {
        //Allocate new page
        size_t ent_size = sizeof(tree_node) + order*sizeof(tree_node::cslot), page_size = sizeof(npool_page) + ent_size * NPOOL_PAGE_SIZE;
        npool_page *page = (npool_page*) new uint8_t[page_size];
        page->next_page = m_NodePoolPages[order];
        m_NodePoolPages[order] = page;
        m_MemUsage += page_size;

        //Fill free list
        npool_ent *prev_free = nullptr;
        for(int i = NPOOL_PAGE_SIZE-1; i >= 0; i--) {
            npool_ent *ent = (npool_ent*) ((uint8_t*) &page->nodes + ent_size*i);
            ent->next_free = prev_free;
            prev_free = ent;
        }
        m_NodePoolFreeHead[order] = prev_free;
    }

    //Unlink from free list
    npool_ent *ent = m_NodePoolFreeHead[order];
    assert(ent);
    m_NodePoolFreeHead[order] = ent->next_free;

    tree_node *node = (tree_node*) ent;
    *node = tree_node(lab_start, lab_end, order);
    node->suf_link = &m_RootNode;
    return node;
}

inline void CSuffixTree::free_node(CSuffixTree::tree_node *node) {
    int order = node->order();
    npool_ent *ent = (npool_ent*) node;

    //Link into free list
    ent->next_free = m_NodePoolFreeHead[order];
    m_NodePoolFreeHead[order] = ent;
}

CSuffixTree::tree_node *CSuffixTree::new_leaf(size_t lab_start) {
    return alloc_node(lab_start, tree_node::LABEL_END_LEAF, 0);
}

CSuffixTree::tree_node *CSuffixTree::new_bifurc(size_t lab_start, size_t lab_end, tree_node *node_a, tree_node *node_b) {
    tree_node *node = alloc_node(lab_start, lab_end, 2);

    //Fill child slots
    uint8_t sym_a = m_Sequence[node_a->lab_start], sym_b = m_Sequence[node_b->lab_start];
    assert(sym_a != sym_b);
    if(sym_a < sym_b) {
        node->children[0] = tree_node::cslot(sym_a, node_a);
        node->children[1] = tree_node::cslot(sym_b, node_b);
    } else {
        node->children[0] = tree_node::cslot(sym_b, node_b);
        node->children[1] = tree_node::cslot(sym_a, node_a);
    }

    return node;
}

CSuffixTree::tree_node *CSuffixTree::add_node_child(tree_node_ref *node, tree_node *child) {
    tree_node *onode = *node;
    assert(onode->is_root() || !onode->is_leaf());

    int order = onode->order();
    uint8_t csym = m_Sequence[child->lab_start];

    //Allocate new node
    tree_node *nnode = alloc_node(onode->lab_start, onode->label_end(*this), order + 1);

    //Insert child
    int ni = 0;
    for(int i = 0; i < order; i++) {
        uint8_t sym = onode->children[i].symbol;
        assert(sym != csym);

        if(csym < sym && i == ni) {
            nnode->children[ni++] = tree_node::cslot(csym, child);
        }
        nnode->children[ni++] = onode->children[i];
    }
    if(ni == order) nnode->children[ni++] = tree_node::cslot(csym, child);
    assert(ni == order+1);

    //Free the old node
    free_node(onode);
    *node = nnode;

    return nnode;
}