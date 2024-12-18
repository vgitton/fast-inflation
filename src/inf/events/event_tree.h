#pragma once

#include "../../util/debug.h"
#include "../../util/file_stream.h"
#include "../../util/loggable.h"
#include "event.h"
#include <memory>

namespace inf {

/*! \ingroup eventstorage
 * \brief Storing a set of inf::Event as a tree
 * \details The idea is that we take a set of inf::Event of fixed length.
 * We then say, for instance: the first outcome of the inf::Event is either 0 or 2.
 * Then, when the first outcome is 0, the second outcome is either 1 or 2, etc.
 * This data structure is used in particular in the Frank-Wolfe algorithm to quickly
 * evaluate inner products with respect to a set of inf::Event.
 *
 * ### Usage
 *
 * \note The below example is included in the user::Application that tests
 * the inf::EventTree class: see user::event_tree.
 *
 * To create a tree equivalent to the one described below, use the following code:
 * \code{.cpp}
 * // Create a tree with depth 3
 * inf::EventTree tree(3);
 * // Insert some nodes
 * tree.insert_node(0, 0, {
 *     tree.insert_node(1, 0, {
 *         tree.insert_node(2, 0, {}),
 *         tree.insert_node(2, 1, {})
 *     }),
 *     tree.insert_node(1, 1, {
 *         tree.insert_node(2, 0, {})
 *     })
 * });
 * tree.insert_node(0, 1, {
 *     tree.insert_node(1, 0, {
 *         tree.insert_node(2, 0, {}),
 *         tree.insert_node(2, 1, {})
 *     }),
 *     tree.insert_node(1, 1, {
 *         tree.insert_node(2, 1, {})
 *     })
 * });
 * // Important: do not forget to call this method after setting up the inf::EventTree!
 * // There are soft assertions that check that this method was indeed called, though.
 * tree.finish_initialization();
 * // Display different views over the inf::EventTree:
 * tree.log_flat();
 * util::logger << tree;
 * // Display size-related information:
 * tree.log_info();
 * \endcode
 *
 * ### Compression algorithm
 *
 * The inf::EventTree data structure is optimized to have a minimal memory footprint.
 * The idea of the compression technique that we use is the following.
 * Consider an inf::EventTree that represents a subset of \f$\events{\{\alice,\bob,\charlie\}}\f$,
 * for \f$\nouts = 2\f$, i.e., a subset of all lists in \f$\{0,1\}^{\times 3}\f$.
 * Suppose that the tree looks as follows:
 * \image html tree_1.png
 * We use the notation `N[d][i] = (a,{N1,N2,...})` to represent the inf::EventTree::Node stored at depth `d` and which has index `i` relative
 * to that depth, and to indicate that this inf::EventTree::Node has outcome `a` and children `N1,N2,...`.
 * Without compression, the above tree would be stored as the following two-dimensional array:
 * ```
 * {
 *     { N[0][0] = (0,{N[1][0],N[1][1]}), N[0][1] = (1,{N[1][2],N[1][3]}) },
 *     { N[1][0] = (0,{N[2][0],N[2][1]}), N[1][1] = (1,{N[2][2]}), N[1][2] = (0,{N[2][3],N[2][4]}), N[1][3] = (1,{N[2][5]}) },
 *     { N[2][0] = (0,{}), N[2][1] = (1,{}), N[2][2] = (0,{}), N[2][3] = (0,{}), N[2][4] = (1,{}), N[2][5] = (1,{}) }
 * }
 * ```
 * However, we can start compressing the last depth. Indeed, although the graph of the tree features six leaves (terminal nodes),
 * there are only two types of terminal nodes here: the one with the 0 outcome and the one with the 1 outcome.
 * Visually, we can represent this compression as follows:
 * \image html tree_2.png
 * Hence, we can compress the representation of the tree as follows:
 * ```
 * {
 *     { N[0][0] = (0,{N[1][0],N[1][1]}), N[0][1] = (1,{N[1][2],N[1][3]}) },
 *     { N[1][0] = (0,{N[2][0],N[2][1]}), N[1][1] = (1,{N[2][0]}), N[1][2] = (0,{N[2][0],N[2][1]}), N[1][3] = (1,{N[2][1]}) },
 *     { N[2][0] = (0,{}), N[2][1] = (1,{}) }
 * }
 * ```
 * Notice how the last depth only has two types of nodes, and it is now the responsibility of the forelast
 * depth to correctly point to those two types of nodes.
 * We can apply the same logic to the forelast depth: indeed, the elements `N[1][0]` and `N[1][2]` are identical.
 * In the original graph, this corresponds to the subtree
 * \image html tree_3.png
 * appearing twice.
 * Thus, we can compress the tree further by removing the duplicate element at depth 1, and updating the pointers at depth 0.
 * This looks as follows visually:
 * \image html tree_4.png
 * And this corresponds to the following compressed data structure:
 * ```
 * {
 *     { N[0][0] = (0,{N[1][0],N[1][1]}), N[0][1] = (1,{N[1][0],N[1][2]}) },
 *     { N[1][0] = (0,{N[2][0],N[2][1]}), N[1][1] = (1,{N[2][0]}), N[1][2] = (1,{N[2][1]}) },
 *     { N[2][0] = (0,{}), N[2][1] = (1,{}) }
 * }
 * ```
 * \warning The implementation of inf::EventTree::Node is such that no node can have more than
 * 256 children.
 * */
class EventTree : public util::Loggable, public util::Serializable {
  public:
    /*! \brief An inf::EventTree can take some time to initialize, and this option is used to control
     * whether an inf::EventTree should be written to or read from disk. */
    enum class IO {
        read,  //!< To read an inf::EventTree from a file
        write, //!< To write an inf::EventTree to a file
        none   //!< To do neither of the above
    };

    static void log(inf::EventTree::IO io);

    /*! \brief This lightweight data structure uniquely identifies a node in some inf::EventTree.
     * \details In the inf::EventTree, the inf::EventTree::Node are stored in a two-dimensional array:
     *  - the first index of this two-dimensional array, the \p depth index, represents the depth (distance)
     * of an inf::EventTree::Node from the root of the inf::EventTree. This \p depth also corresponds to the position in the
     * list of outcomes in which the outcome of the inf::EventTree::Node should be placed. This position can be thought of
     * as the identifier of some inflation party.
     *  - then, the second index of this two-dimensional array, the \p node_index index, identifies the inf::EventTree::Node
     *  within that \p depth. */
    class NodePos {
      public:
        /*! \brief Iterating over each inf::EventTree::Node stored in a inf::EventTree requires a queue/stack to keep
         * track of which branches remain to be explored. */
        typedef std::vector<inf::EventTree::NodePos> Queue;

        /*! \brief The depth is stored to know to which inflation party the node corresponds to
            \param depth the depth (corresponding to an inflation party) of the inf::EventTree::Node from the root of the inf::EventTree
            \param node_index Essentially a pointer to a inf::EventTree::Node stored in the inf::EventTree */
        NodePos(Index depth, Index node_index);
        /*! \brief the depth (corresponding to an inflation party) of the inf::EventTree::Node from the root of the inf::EventTree */
        Index const depth;
        /*! \brief Essentially a pointer to a inf::EventTree::Node stored in the inf::EventTree */
        Index const node_index;
    };

    /*! \brief A node of the inf::EventTree, storing an outcome and some other inf::EventTreee::Node as children
     * \note This class is meant to be lightweight, which is why we want to avoid making it a virtual class.
     * Hence, although an inf::EventTree::Node is effectively a util::Serializable, we do not make it explicitly so.
     * (The reason is that a virtual class instance takes up a tiny bit more memory in C++.)
     * */
    class Node {
      public:
        /*! \brief Default constructor with no children */
        Node(inf::Outcome const outcome = 0);
        /*! \brief Constructor with outcome and children
         * \param outcome The value stored by the node
         * \param children_vec The children are represented as indices which makes sense with respect to the underlying inf::EventTree. */
        Node(inf::Outcome const outcome,
             std::vector<Index> const &children_vec);

        Node(Node const &other);
        Node &operator=(Node const &other);

        Node(Node &&other);
        Node &operator=(Node &&other);

        ~Node();

      private:
        /*! \brief Duplicating children: copy the parameters of this function to the member variables */
        void copy_children(inf::Outcome const target_n_children, Index const *target_children);
        /*! \brief Adopting children: move the children of \p other to the member variables */
        void move_children(Node &&other);
        /*! \brief Frees the \p children member pointer
         * \warning This does not reset the member \p n_children to zero. */
        void clean_children();

      public:
        /*! \brief The outcome stored by the inf::EventTree::Node */
        inf::Outcome outcome;
        /*! \brief The number of children of the inf::EventTree::Node
         * \warning The type of the number of children forbids the inf::EventTree to have a inf::EventTree::Node with more
         * than 256 children. */
        inf::Outcome n_children;
        /*! \brief The children of the inf::EventTree::Node, represented as indices in the underlying inf::EventTree
         * \details These are stored as a C-style array whose size is given by the member \p n_children */
        Index *children;

        /*! \brief An estimate of the number of bytes that a specific inf::EventTree::Node occupies
         * \details In particular, this memory footprint depends on the number of children of the inf::EventTree::Node. */
        Index get_memory_footprint() const;

        /*! \brief We implement comparison operators of inf::EventTree::Node to be able to remove duplicates. */
        bool operator==(Node const &other) const;
        /*! \brief We implement comparison operators of inf::EventTree::Node to be able to remove duplicates. */
        bool operator!=(inf::EventTree::Node const &other) const;

        /*! \brief To serialize (read/write to disk) the inf::EventTree::Node
         * \details We do not want to make this a virtual function to spare a bit of memory for each inf::EventTree::Node instance. */
        void io(util::FileStream &stream);
    };

    typedef std::unique_ptr<inf::EventTree> UniquePtr;

    /*! \brief Construct an empty inf::EventTree, initialized with a fixed \p depth */
    EventTree(Index const depth);
    //! \cond
    EventTree(EventTree const &) = delete;
    EventTree(EventTree &&) = delete;
    EventTree &operator=(EventTree const &) = delete;
    EventTree &operator=(EventTree &&) = delete;
    //! \endcond

    /*! \brief This logs a flat view (close to the internal storage data structure) of the inf::EventTree to the global \p util::logger */
    void log_flat() const;
    /*! \brief This logs a tree-like view of the inf::EventTree to the global \p util::logger */
    void log() const override;
    /*! \brief This displays some key properties of the inf::EventTree, including its number of leaves and estimated memory footprint */
    void log_info() const;
    /*! \brief This logs a tree-like view of the subtree starting at the inf::EventTree::Node identified by \p depth and \p node
     * \sa inf::EventTree::NodePos for an explanation of the meaning of the parameters */
    void log_node_recursive(Index const depth, Index node) const;

    /*! \brief To populate the tree using the compression algorithm detailed above.
     * \details This checks if the inf::EventTree::Node already exists in the inf::EventTree.
     * - If it does, the method simply returns the ::Index of this existing inf::EventTree::Node within the internal list `m_node_cache[depth]`.
     * - If it does not, the method appends a new inf::EventTree::Node at the end of `m_node_cache[depth]`, and return the corresponding ::Index.
     * The returned ::Index allows the user to create parents of this node.
     * */
    Index insert_node(Index const depth, inf::Outcome const outcome, std::vector<Index> const &children);

    /*! \brief This method needs to be called after inserting all the nodes of the tree
     * \details This method initializes the relevant metadata (size, etc) of the inf::EventTree,
     * and optimizes the memory that the inf::EventTree uses.
     * The idea of this is that the user should first insert the relevant
     * nodes with inf::EventTree::insert_node(), and then call inf::EventTree::finish_initialization().
     * The rest of the class features some soft assertions making sure that this method was indeed called. */
    void finish_initialization();

    /*! \brief Estimated amount of memory (RAM) that the inf::EventTree uses */
    Index get_memory_footprint() const;

    /*! \brief The distance of the leaves to the root of the tree. Equivalently, the length of the underlying inf::Event set. */
    Index get_depth() const;
    /*! \brief The number of leaves (counting duplicates) of the tree, i.e., the number of nodes without children.
     * \details This is the number of leaves of the graph. For instance, the binary tree
     * encoding the set \f$\{0,1\}^{\times 3}\f$ has 8 leaves, although it has only two types of
     * nodes without children (the 0 and the 1 node). */
    Index get_n_leaves() const;
    /*! \brief The number of unique nodes that the tree stores
     * \details Thanks to the compression algorithm, this can be arbitrarily fewer
     * nodes than the number of leaves obtained with inf::EventTree::get_n_leaves().
     * For instance, for the binary tree encoding the set \f$\{0,1\}^{\times 3}\f$,
     * this would be \f$2 + 2 + 2 = 6\f$ unique nodes. */
    Index get_n_nodes_cached() const;
    /*! \brief The total number of nodes (counting duplicates) that the tree stores
     * \details For instance, for the binary tree encoding the set \f$\{0,1\}^{\times 3}\f$,
     * this would be \f$2 + 4 + 8 = 14\f$ nodes. */
    Index get_n_nodes_total() const;

    /*! \brief This method is typically used together with an inf::EventTree::NodePos::Queue to traverse the inf::EventTree. */
    inf::EventTree::Node const &get_node(inf::EventTree::NodePos const &node_pos) const {
        ASSERT_LT(node_pos.depth, m_depth)
        ASSERT_LT(node_pos.node_index, m_node_cache[node_pos.depth].size())
        return m_node_cache[node_pos.depth][node_pos.node_index];
    }

    /*! \brief The number of root children, i.e., of nodes that connect directly to the root of the tree.
     * \details In the case of the binary tree representing the set \f$\{0,1\}^{\times3}\f$, this would be 2. */
    Index get_root_children_count() const {
        return m_node_cache[0].size();
    }

    /*! \brief Returns the root children as a queue to start traversing the tree */
    inf::EventTree::NodePos::Queue get_root_children_queue() const {
        inf::EventTree::NodePos::Queue queue;
        Index const n_root_children = get_root_children_count();

        for (Index node_index(0); node_index < n_root_children; ++node_index) {
            queue.emplace_back(0, node_index);
        }

        return queue;
    }

    /*! \brief Returns the breadth (number of nodes, counting duplicates) of the tree at \p depth
     * \details In the case of the binary tree representing the set \f$\{0,1\}^{\times3}\f$, this would be
     * 2 at `depth = 0`, 4 at `depth = 1` and 8 at `depth = 2`. */
    Index get_breadth_at_depth(Index const depth) const;

    /*! \brief Returns the number of leaves (counting duplicates) of the subtree that includes all descendants of the given node */
    Index count_leaves_from(inf::EventTree::NodePos const &node_pos) const;

    /*! \brief Appends to \p queue the children of the inf::EventTree::Node indicated by \p node_pos */
    void add_children_to_queue(inf::EventTree::NodePos::Queue &queue, inf::EventTree::NodePos const &node_pos) const {

        inf::EventTree::Node const &the_node = get_node(node_pos);
        Index const n_children = the_node.n_children;
        Index const *const children = the_node.children;
        Index const new_depth = node_pos.depth + 1;

        for (Index child_node_index(0); child_node_index < n_children; ++child_node_index)
            queue.emplace_back(new_depth, children[child_node_index]);
    }

    /*! \brief This method allows to check that inf::EventTree::finish_initialization() was properly called
     * \details The idea of this is that the user should first insert the relevant
     * nodes with inf::EventTree::insert_node(), and then call inf::EventTree::finish_initialization(). */
    bool is_initialized() const;

    // For serialization
    void io(util::FileStream &stream) override;

    bool operator==(inf::EventTree const &other) const;
    bool operator!=(inf::EventTree const &other) const;

  private:
    /*! \brief The length from root to any leaf */
    Index const m_depth;
    /*! \brief We store one list of unique nodes per depth
     * \details See the general class description for an explanation of the compression
     * algorithm underlying this data structure.
     * The root children are stored in `m_node_cache[0]`, while the unique leaves
     * are stored in `m_node_cache.back()`. */
    std::vector<std::vector<inf::EventTree::Node>> m_node_cache;

    /*! \brief The idea of this test is that the user should first insert the relevant
     * nodes with inf::EventTree::insert_node(), and then call inf::EventTree::finish_initialization(). */
    bool m_finished_initialization;

    /*! \brief An estimate of the RAM that the inf::EventTree uses */
    Index m_memory_footprint;
    /*! \brief The number of leaves, see inf::EventTree::get_n_leaves() */
    Index m_n_leaves;
    /*! \brief The number of nodes, see inf::EventTree::get_n_nodes_total() */
    Index m_n_nodes_total;
    /*! \brief This method is called by inf::EventTree::finish_initialization() to initialize the different size-related
     * fields of the inf::EventTree. */
    void init_info();
};

} // namespace inf
