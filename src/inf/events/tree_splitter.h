#pragma once

#include "event_tree.h"

namespace inf {

/*! \ingroup eventstorage
 * \brief This class allows the user to partition an inf::EventTree into a group of
 * subtrees that each have roughly the same number of leaves.
 * \details This is useful for a multithreaded traversal of an inf::EventTree.
 * To make sure that each threads gets roughly the same amount of work,
 * it makes sense to divide the tree into subtrees that have the same amount of leaves
 * (assuming that the work that each thread does scales with the number of leaves of the tree).
 *
 * \note This class is tested in a dedicated user::Application: see user::tree_splitter.
 *
 * ### Idea of the split
 *
 * Let us assume no compression in the inf::EventTree, so that we simply store inf::EventTree::Node
 * as `N[d][i]`, where `d` is the depth of the node and `i` is the index of the node at that depth.
 * Suppose that we want to split the following tree in two:
 * \image html tree_5.png
 * We see that splitting at depth 0 results in two subtrees with 1 and 3 leaves, respectively, which is unbalanced.
 * To measure the amount of imbalance, we use the ratio of \f$n_\text{max} / n_\text{min} \f$,
 * where \f$n_\text{max}\f$ (\f$n_\text{min}\f$) is the number of leaves of the subtree with the most (fewest) leaves.
 * In this case, the quality factor would be 3, which is too high.
 * We can instead try to split at depth 1.
 * In this case, the split at depth 1 can be described as follows:
 * \code{.cpp}
 * inf::TreeSplitter::PathPartition path_partition = {
 *     // First subtree with 2 leaves
 *     {
 *         // First path to explore
 *         {0, 0}, // Meaning: N[0][0] then N[1][0]. This path has 1 leaf.
 *         // Second path to explore
 *         {1, 1} // Meaning: N[0][1] then N[1][1]. This path has 1 leaf.
 *     },
 *     // Second subtree with 2 leaves
 *     {
 *         // First path to explore
 *         {1, 2} // Meaning: N[0][1] then N[1][2]. This path has 2 leaves.
 *     }
 * }
 * \endcode
 * Since each subtree has two leaves, the quality factor is 1 in this case (a perfect split).
 * In practice, we aim for a quality factor of at most inf::TreeSplitter::target_quality_factor.
 * The above example assumes no compression, but the compression algorithm of the inf::EventTree
 * simply amounts to a different way of labeling each node, which does not change the reasoning
 * of how to perform the split.
 * To see this example in action with the compression algorithm, please refer to the test user::event_tree.
 * */
class TreeSplitter {
  public:
    /*! \brief A path from the root to a (typically non-terminal) node of an inf::EventTree */
    typedef std::vector<Index> Path;

    /*! \brief We will look at the set of paths that finish at a certain depth, and we need to know
     * the number of (non-unique) leaves that each path contains */
    struct PathAndLeaves {
        Path path;
        Index n_leaves;
    };

    /*! \brief A path partition is a list of subtrees, where a subtree is represented a list of inf::TreeSplitter::Path. */
    typedef std::vector<std::vector<Path>> PathPartition;
    typedef std::shared_ptr<PathPartition> PathPartitionPtr;
    typedef std::shared_ptr<const PathPartition> PathPartitionConstPtr;

    /*! \brief This is an upper bound on the ratio \f$n_\text{max} / n_\text{min} \f$,
     * where \f$n_\text{max}\f$ (\f$n_\text{min}\f$) is the number of leaves of the subtree with the most (fewest) leaves */
    static double const target_quality_factor;

    /*! \brief The main method of the class that the user should call to divide an inf::EventTree \p event_tree
     * approximately equally in \p n_splits many subtrees
     * \details This function will throw errors if it does not manage to find a suitable split. */
    static PathPartitionConstPtr get_path_partition(inf::EventTree const &event_tree, Index const n_splits);

  private:
    /*! \brief The constructor is private because this class is meant to only instantiate temporary objects,
     * use inf::TreeSplitter::get_path_partition() instead */
    TreeSplitter(inf::EventTree const &event_tree, Index const n_splits);

    /*! \brief To obtain the path partition after computing it */
    PathPartitionConstPtr get_path_partition() const {
        return m_path_partition;
    }

    /*! \brief This method returns true if the split was successful, in which case \p m_path_partition is appropriately set, else returns false */
    bool try_to_split_at_depth(Index const depth);

    /*! \brief This method returns all the paths at the given depth, ordered according to the number of leaves they carry.
     * The number of leaves they each carry is also returned.
     * \param depth
     * \param breadth The method inf::EventTree::get_breadth_at_depth() can be a bit slow, and it was already invoked
     * before we call this method, so we simply pass the \p breadth as a parameter instead of recomputing it. */
    std::vector<inf::TreeSplitter::PathAndLeaves> get_paths_and_leaves(Index const depth,
                                                                       Index const breadth) const;

    /*! \brief Attemps to divide the \p paths_and_leaves vector into almost-equal (in terms of number of leaves)
     * consecutive lists.
     * \param splits Can be passed as empty list. This list will hold the start position of each sublist relative to the indices of \p paths_and_leaves.
     * The split number `i` refers to `paths_and_leaves[splits[i]]` to `paths_and_leaves[splits[i+1]-1]`, hence
     * `splits[0]` is always 0 and `splits.back()` is always `paths_and_leaves.size()-1`.
     * \param paths_and_leaves The list of paths at a certain depth. Ordering this list (before passing it as parameter) by the number of leaves of the paths
     * helps finding a better split. */
    bool find_splits(std::vector<Index> &splits,
                     std::vector<PathAndLeaves> const &paths_and_leaves) const;

    // Data

    /*! \brief The inf::EventTree to split
     * \details Only required to be a valid reference during the invocation of the constructor. */
    inf::EventTree const &m_event_tree;
    /*! \brief The size of the desired partition of the inf::EventTree. Typically, this corresponds to the number of threads of the program. */
    Index const m_n_splits;
    /*! \brief The ratio between the max and min number of leaves per subtree of the partition. Should be close to 1.0. */
    double m_quality_factor;
    /*! \brief A list of size \p m_n_splits, each of which is a list of Path describing a subtree.
     * \details When multithreading, each thread receives one element of this list to investigate. */
    PathPartitionPtr m_path_partition;
    /*! \brief This list takes the form {0, n, 2n, ...} where n is the hoped-for number of leaves per subtree.
     * The first element, 0, is for more convenient indexing. */
    std::vector<Index> m_leave_thresholds;
};

} // namespace inf
