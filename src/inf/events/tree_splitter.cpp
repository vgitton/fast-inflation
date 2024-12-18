#include "tree_splitter.h"
#include "../../util/logger.h"
#include "../../util/misc.h"

// For std::minmax_element and std::sort
#include <algorithm>

// Allow for at most 10% more work between two threads
double const inf::TreeSplitter::target_quality_factor = 1.1;
// Let's see if this changes a lot? -> it doesn't
// double const inf::TreeSplitter::target_quality_factor = 1.3;

inf::TreeSplitter::PathPartitionConstPtr inf::TreeSplitter::get_path_partition(
    const inf::EventTree &event_tree,
    const Index n_splits) {

    return inf::TreeSplitter(event_tree, n_splits).get_path_partition();
}

inf::TreeSplitter::TreeSplitter(inf::EventTree const &event_tree, Index const n_splits)
    : m_event_tree(event_tree),
      m_n_splits(n_splits),
      m_quality_factor(0.0),
      m_path_partition(nullptr),
      m_leave_thresholds{} {

    ASSERT_TRUE(m_event_tree.is_initialized())

    // The treatment of 1 split is trivial: just add all the root children
    if (m_n_splits == 1) {

        m_path_partition = std::make_shared<PathPartition>(m_n_splits);
        // Only one subset
        std::vector<Path> &the_paths = (*m_path_partition)[0];

        Index const n_root_children = m_event_tree.get_root_children_count();

        // Each path has length 1
        the_paths = std::vector<Path>(n_root_children);

        (*m_path_partition)[0] = std::vector<Path>(n_root_children);
        for (Index i : util::Range(n_root_children))
            the_paths[i] = Path({i});

        return;
    }

    m_leave_thresholds = std::vector<Index>(m_n_splits);
    for (Index i : util::Range(m_n_splits)) {
        m_leave_thresholds[i] = i * m_event_tree.get_n_leaves() / m_n_splits;
    }

    Index depth = 0;

    while (true) {
        if (depth == event_tree.get_depth())
            THROW_ERROR("TreeSplitter could not find a splitting strategy. Try reducing the number of threads.")

        if (try_to_split_at_depth(depth))
            break;

        ++depth;
    }

    LOG_BEGIN_SECTION("Event tree splitting")
    util::logger << "Found a satisfactory split at depth " << depth << util::cr;
    Index const qf_permill = static_cast<Index>(1000.0 * (m_quality_factor - 1.0));
    util::logger << "The slowest thread has "
                 << qf_permill / 10 << "." << qf_permill % 10 << "%"
                 << " more leaves to process than the fastest thread." << util::cr;
    LOG_END_SECTION
}

bool inf::TreeSplitter::try_to_split_at_depth(Index const depth) {

    Index const breadth = m_event_tree.get_breadth_at_depth(depth);

    // Need the breadth to be at least as large as the number of splits to have
    // a non-empty set of paths to look at by each thread
    if (breadth < m_n_splits)
        return false;

    std::vector<PathAndLeaves> paths_and_leaves = get_paths_and_leaves(depth, breadth);
    ASSERT_EQUAL(paths_and_leaves.size(), breadth)

    // This list contains {0, i1, i2, i3, breadth}
    // where the first thread looks at [0,i1), the second at [i1,i2), etc.
    std::vector<Index> splits;
    if (not find_splits(splits, paths_and_leaves)) {
        // The splits could not be found at this depth, e.g., because the number of leaves of the paths
        // at this depth look like (1,1,1,...,1,very large number), and so the first subtree took all
        // paths for itself, which doesn't make sense
        return false;
    }
    // splits now contain the best splits that we could find at this depth.
    // Let's check how good this splitting is.
    // The number of leaves per split is the metric to check that the split was fairly even.
    std::vector<Index> leaves_per_split(m_n_splits, 0);
    for (Index split_index : util::Range(m_n_splits)) {
        for (Index path_index : util::Range(splits[split_index],
                                            splits[split_index + 1])) {
            leaves_per_split[split_index] += paths_and_leaves[path_index].n_leaves;
        }
    }
    // Compute the quality factor
    {
        auto const min_max = std::minmax_element(leaves_per_split.begin(), leaves_per_split.end());
        m_quality_factor = static_cast<double>(*min_max.second) / static_cast<double>(*min_max.first);
        ASSERT_LTE(1.0, m_quality_factor)
    }
    // The success of the split depends on how low the quality factor is
    if (m_quality_factor > inf::TreeSplitter::target_quality_factor) {
        // The quality factor is not good enough
        return false;
    }

    // Display
    for (Index i : util::Range(m_n_splits)) {
        util::logger
            << "Thread " << i + 1
            << " gets the range [" << splits[i] << "," << splits[i + 1] << ") "
            << "with " << leaves_per_split[i] << " leaves" << util::cr;
    }

    // We have found a good split! We can set m_path_partition accordingly.
    m_path_partition = std::make_shared<PathPartition>(m_n_splits);

    for (Index split_index : util::Range(m_n_splits)) {
        ASSERT_LT(splits[split_index], splits[split_index + 1])
        Index const split_size = static_cast<Index>(splits[split_index + 1] - splits[split_index]);
        (*m_path_partition)[split_index] = std::vector<Path>(split_size);

        for (Index path_index : util::Range(split_size)) {
            (*m_path_partition)[split_index][path_index] = paths_and_leaves[splits[split_index] + path_index].path;
        }
    }

    return true;
}

std::vector<inf::TreeSplitter::PathAndLeaves> inf::TreeSplitter::get_paths_and_leaves(
    Index const depth,
    Index const breadth) const {

    ASSERT_LT(depth, m_event_tree.get_depth())
    ASSERT_EQUAL(breadth, m_event_tree.get_breadth_at_depth(depth))

    // Initialize paths_and_leaves to a list of length breadth with all paths set to the right
    // size but with all zero indices for now. Furthermore, initialize the number of leaves to zero.
    std::vector<PathAndLeaves> paths_and_leaves(breadth, {std::vector<Index>(depth + 1), 0});

    // Initialize paths_and_leaves
    Index path_index = 0;
    inf::EventTree::NodePos::Queue queue = m_event_tree.get_root_children_queue();
    while (not queue.empty()) {
        inf::EventTree::NodePos node_pos = util::pop_back(queue);

        // This method does a bit too many calculations, I guess, but it works
        for (Index other_path_index : util::Range(path_index, breadth)) {
            paths_and_leaves[other_path_index].path[node_pos.depth] = node_pos.node_index;
        }

        if (node_pos.depth == depth) {
            // We reached the end of this path. Count leaves and move on.
            paths_and_leaves[path_index].n_leaves = m_event_tree.count_leaves_from(node_pos);
            ++path_index;
        } else {
            m_event_tree.add_children_to_queue(queue, node_pos);
        }
    }

    // We sort the paths by their number of leaves, this allows a smoother splitting later
    std::sort(paths_and_leaves.begin(),
              paths_and_leaves.end(),
              [](PathAndLeaves const &first, PathAndLeaves const &second) {
                  return first.n_leaves < second.n_leaves;
              });

    // util::logger << util::cr << "All the paths are:" << util::cr;
    // for (PathAndLeaves const &path : path_and_leaves) {
    //     util::logger << path.path << " -> " << path.n_leaves << util::cr;
    // }

    return paths_and_leaves;
}

bool inf::TreeSplitter::find_splits(std::vector<Index> &splits,
                                    std::vector<PathAndLeaves> const &paths_and_leaves) const {

    Index const breadth = paths_and_leaves.size();

    // This list contains {0, i1, i2, i3, breadth}
    // where the first thread looks at [0,i1), the second at [i1,i2), etc.
    splits = std::vector<Index>(m_n_splits + 1);
    splits[0] = 0;
    splits.back() = breadth;

    Index accumulated_leaves = 0;
    Index split_index = 1;

    for (Index path_index : util::Range(breadth)) {

        accumulated_leaves += paths_and_leaves[path_index].n_leaves;

        if (accumulated_leaves > m_leave_thresholds[split_index]) {
            // We cannot have i1 = 0, this would mean the first thread gets no leaves!
            if (path_index == 0)
                return false;

            splits[split_index] = path_index;
            ++split_index;

            // If we found all our splits, we exit. This condition is necessary, for
            // otherwise we'd be trying to set up splits.back(), which doesn't make sense
            if (split_index == splits.size() - 1)
                break;
        }
    }

    // Normally, we should have set all the way up to i3, leaving split_index to 4.
    // If not, this means that the accumulated leaves could look for instance like {1,1,1,1,1,1,1,(large number)}.
    // Then i1 = 7 and we exit the above loop without having set the other i's.
    return split_index == splits.size() - 1;
}
