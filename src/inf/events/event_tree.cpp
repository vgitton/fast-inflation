#include "event_tree.h"

#include "../../util/debug.h"
#include "../../util/logger.h"
#include "../../util/misc.h"

inf::EventTree::NodePos::NodePos(Index const depth, Index const node_index)
    : depth(depth), node_index(node_index) {}

void inf::EventTree::log(inf::EventTree::IO io) {
    util::logger << util::begin_comment << "inf::EventTree::IO::" << util::end_comment;
    switch (io) {
    case inf::EventTree::IO::read:
        util::logger << "read";
        break;
    case inf::EventTree::IO::write:
        util::logger << "write";
        break;
    case inf::EventTree::IO::none:
        util::logger << "none";
        break;
    default:
        THROW_ERROR("switch")
    }
}

// Node

inf::EventTree::Node::Node(inf::Outcome const outcome)
    : outcome(outcome), n_children(0), children(nullptr) {}

inf::EventTree::Node::Node(inf::Outcome const outcome, std::vector<Index> const &children_vec)
    : inf::EventTree::Node::Node(outcome) {
    copy_children(static_cast<inf::Outcome>(children_vec.size()), children_vec.data());
}

inf::EventTree::Node::Node(inf::EventTree::Node const &other)
    : inf::EventTree::Node::Node(other.outcome) {
    copy_children(other.n_children, other.children);
}

inf::EventTree::Node &inf::EventTree::Node::operator=(inf::EventTree::Node const &other) {
    outcome = other.outcome;
    copy_children(other.n_children, other.children);
    return *this;
}

inf::EventTree::Node::Node(inf::EventTree::Node &&other)
    : inf::EventTree::Node::Node(other.outcome) {
    move_children(std::move(other));
}

inf::EventTree::Node &inf::EventTree::Node::operator=(inf::EventTree::Node &&other) {
    outcome = other.outcome;
    move_children(std::move(other));

    return *this;
}

inf::EventTree::Node::~Node() {
    clean_children();
}

void inf::EventTree::Node::copy_children(inf::Outcome const target_n_children, Index const *target_children) {
    clean_children();

    n_children = target_n_children;

    if (n_children > 0) {
        children = new Index[n_children];
        for (Index i(0); i < n_children; ++i) {
            children[i] = target_children[i];
        }
    }
}

void inf::EventTree::Node::move_children(inf::EventTree::Node &&other) {
    clean_children();

    n_children = other.n_children;
    children = other.children;

    other.clean_children();
}

void inf::EventTree::Node::clean_children() {
    if (n_children > 0) {
        delete[] children;
        children = nullptr;
    }
}

Index inf::EventTree::Node::get_memory_footprint() const {
    return sizeof(inf::EventTree::Node) + n_children * sizeof(Index);
}

bool inf::EventTree::Node::operator==(inf::EventTree::Node const &other) const {
    if (outcome != other.outcome)
        return false;

    if (n_children != other.n_children)
        return false;

    for (Index i : util::Range(n_children)) {
        if (children[i] != other.children[i])
            return false;
    }

    return true;
}

bool inf::EventTree::Node::operator!=(inf::EventTree::Node const &other) const {
    return not(*this == other);
}

void inf::EventTree::Node::io(util::FileStream &stream) {
    if (stream.is_reading() and n_children > 0)
        delete[] children;

    stream.io(outcome);
    stream.io(n_children);

    if (stream.is_reading() and n_children > 0)
        children = new Index[n_children];

    for (Index i : util::Range(n_children)) {
        stream.io(children[i]);
    }
}

// Tree

inf::EventTree::EventTree(Index const depth)
    : m_depth(depth), m_node_cache(depth), m_finished_initialization(false),
      m_memory_footprint(0), m_n_leaves(0), m_n_nodes_total(0) {
    ASSERT_LT(0, depth)
    // This doesn't seem to help...
    // m_node_cache.reserve(91418);
}

void inf::EventTree::log_flat() const {
    ASSERT_TRUE(m_finished_initialization)

    LOG_BEGIN_SECTION("inf::EventTree (flat)")

    for (Index depth : util::Range(m_depth)) {
        for (Index node_index : util::Range(m_node_cache[depth].size())) {

            inf::EventTree::Node const &the_node = m_node_cache[depth][node_index];
            util::logger << "N[" << depth << "," << node_index << "] : ";
            util::logger.echo_colored_number(
                static_cast<int>(the_node.outcome));
            util::logger << " -> (";
            for (Index child_i : util::Range(the_node.n_children)) {
                if (child_i > 0)
                    util::logger << ", ";
                util::logger << "N[" << depth + 1 << ","
                             << the_node.children[child_i] << "]";
            }
            util::logger << ")" << util::cr;
        }
    }

    LOG_END_SECTION
}

void inf::EventTree::log() const {
    ASSERT_TRUE(m_finished_initialization)

    LOG_BEGIN_SECTION("inf::EventTree")

    util::logger << util::begin_section;
    for (Index root_child_i : util::Range(m_node_cache[0].size())) {
        this->log_node_recursive(0, root_child_i);
    }
    util::logger << util::end_section;

    LOG_END_SECTION
}

void inf::EventTree::log_info() const {
    ASSERT_TRUE(m_finished_initialization)

    LOG_BEGIN_SECTION("inf::EventTree")

    Index n_leaves = get_n_leaves();
    LOG_VARIABLE(n_leaves)
    Index n_nodes_total = get_n_nodes_total();
    LOG_VARIABLE(n_nodes_total)
    Index n_nodes_cached = get_n_nodes_cached();
    LOG_VARIABLE(n_nodes_cached)

    // Memory
    Index estimated_ram_bytes = get_memory_footprint();
    LOG_VARIABLE(estimated_ram_bytes)
    Index estimated_ram_bytes_per_leave = 1 + estimated_ram_bytes / n_leaves;
    LOG_VARIABLE(estimated_ram_bytes_per_leave)

    bool show_nodes_per_depth = false;
    if (show_nodes_per_depth) {
        for (Index depth : util::Range(m_depth)) {
            util::logger << util::begin_comment << "Depth " << util::end_comment
                         << depth << util::begin_comment << ": "
                         << util::end_comment << m_node_cache[depth].size()
                         << util::begin_comment << " cached nodes"
                         << util::end_comment << util::cr;
        }
    }

    LOG_END_SECTION
}

void inf::EventTree::log_node_recursive(Index const depth,
                                        Index const node_index) const {
    ASSERT_LT(depth, m_depth)

    inf::EventTree::Node const &the_node = m_node_cache[depth][node_index];
    util::logger.echo_colored_number(static_cast<int>(the_node.outcome));
    util::logger << util::cr;
    if (the_node.n_children != 0) {
        util::logger << util::begin_section;
        for (Index child_i : util::Range(the_node.n_children)) {
            this->log_node_recursive(depth + 1, the_node.children[child_i]);
        }
        util::logger << util::end_section;
    }
}

Index inf::EventTree::insert_node(Index const depth, inf::Outcome const outcome,
                                  std::vector<Index> const &children_vec) {
    ASSERT_TRUE(not m_finished_initialization)
    ASSERT_LT(depth, m_depth)

    // util::logger << "Entering insert_node..." << util::cr;
    // LOG_VARIABLE(depth)
    // LOG_VARIABLE(outcome)
    // LOG_VARIABLE(children_vec)

    std::vector<inf::EventTree::Node> &current_node_cache = m_node_cache[depth];
    Index const node_cache_size = current_node_cache.size();
    inf::Outcome const children_vec_size = static_cast<inf::Outcome>(children_vec.size());
    // For performance
    bool equal;
    inf::Outcome node_i;

    for (Index pos(0); pos < node_cache_size; ++pos) {
        // Seems slightly more efficient to remove this?
        // inf::EventTree::Node const& node = current_node_cache[pos];

        if (current_node_cache[pos].outcome == outcome and
            current_node_cache[pos].n_children == children_vec_size) {
            equal = true;
            for (node_i = 0; node_i < children_vec_size; ++node_i) {
                if (current_node_cache[pos].children[node_i] !=
                    children_vec[node_i]) {
                    equal = false;
                    break;
                }
            }
            if (equal)
                return pos;
        }
    }

#ifndef NDEBUG
    for (Index node_index : children_vec) {
        ASSERT_LT(node_index, m_node_cache[depth + 1].size())
    }
#endif

    // If we didn't find the node, we create a new one
    current_node_cache.emplace_back(outcome, children_vec);
    return node_cache_size; // or current_node_cache.size() - 1
}

void inf::EventTree::finish_initialization() {
    ASSERT_TRUE(not m_finished_initialization)

    m_node_cache.shrink_to_fit();
    for (std::vector<inf::EventTree::Node> &node_cache : m_node_cache) {
        node_cache.shrink_to_fit();
    }

    init_info();

    m_finished_initialization = true;
}

Index inf::EventTree::get_memory_footprint() const {
    return m_memory_footprint;
}

Index inf::EventTree::get_depth() const { return m_depth; }

Index inf::EventTree::get_n_leaves() const { return m_n_leaves; }

Index inf::EventTree::get_n_nodes_cached() const {
    Index ret = 0;
    for (std::vector<inf::EventTree::Node> const &node_cache : m_node_cache) {
        ret += node_cache.size();
    }
    return ret;
}

Index inf::EventTree::get_n_nodes_total() const { return m_n_nodes_total; }

Index inf::EventTree::get_breadth_at_depth(Index const depth) const {
    ASSERT_LT(depth, get_depth())

    Index breadth = 0;

    inf::EventTree::NodePos::Queue queue = get_root_children_queue();

    while (not queue.empty()) {
        inf::EventTree::NodePos node_pos = util::pop_back(queue);

        if (node_pos.depth == depth) {
            ++breadth;
        } else if (node_pos.depth < depth) {
            add_children_to_queue(queue, node_pos);
        }
    }

    return breadth;
}

Index inf::EventTree::count_leaves_from(inf::EventTree::NodePos const &initial_node_pos) const {

    Index ret = 0;

    inf::EventTree::NodePos::Queue queue{initial_node_pos};

    // Our below logic assumes at least depth 2
    ASSERT_LT(1, get_depth())

    while (not queue.empty()) {
        inf::EventTree::NodePos node_pos = util::pop_back(queue);

        // If at forelast depth
        if (node_pos.depth == get_depth() - 1)
            ret += 1;
        else
            add_children_to_queue(queue, node_pos);
    }

    return ret;
}

bool inf::EventTree::is_initialized() const {
    return m_finished_initialization;
}

void inf::EventTree::io(util::FileStream &stream) {
    if (not stream.is_reading()) {
        HARD_ASSERT_TRUE(is_initialized())
    }

    // 1 - Version
    {
        Index const expected_version = 2;
        stream.write_or_read_and_hard_assert(expected_version);
    }

    // 2 - Node cache
    {
        stream.io(m_node_cache);

        if (stream.is_reading()) {
            HARD_ASSERT_EQUAL(m_depth, m_node_cache.size())

            for (Index depth : util::Range(m_depth)) {
                std::vector<inf::EventTree::Node> const &node_cache_row =
                    m_node_cache[depth];

                HARD_ASSERT_LT(0, node_cache_row.size())

                for (inf::EventTree::Node const &node : node_cache_row) {
                    if (depth == m_depth - 1) {
                        // We expect childless nodes here
                        HARD_ASSERT_EQUAL(node.n_children, 0)
                    } else {
                        // We can check that the child indices
                        // make sense
                        for (Index child_i : util::Range(node.n_children))
                            HARD_ASSERT_LT(node.children[child_i],
                                           m_node_cache[depth + 1].size())
                    }
                }
            }
        }
    }

    // 3 - Info
    {
        if (stream.is_reading()) {
            init_info();
        }
    }

    if (stream.is_reading())
        m_finished_initialization = true;
}

bool inf::EventTree::operator==(inf::EventTree const &other) const {
    if (get_depth() != other.get_depth())
        return false;

    if (not is_initialized())
        return false;

    if (not other.is_initialized())
        return false;

    for (Index depth : util::Range(get_depth())) {
        if (m_node_cache[depth].size() != other.m_node_cache[depth].size())
            return false;

        for (Index node_i : util::Range(m_node_cache[depth].size())) {
            if (m_node_cache[depth][node_i] !=
                other.m_node_cache[depth][node_i])
                return false;
        }
    }

    return true;
}

bool inf::EventTree::operator!=(inf::EventTree const &other) const {
    return not(*this == other);
}

void inf::EventTree::init_info() {
    // sizeof(inf::EventTree) accounts for the std::vector m_node_cache and the
    // root index
    m_memory_footprint = sizeof(inf::EventTree) +
                         m_depth * sizeof(std::vector<inf::EventTree::Node>);

    inf::EventTree::NodePos::Queue queue = get_root_children_queue();

    // Number of leaves
    m_n_leaves = 0;
    for (inf::EventTree::NodePos const &root_node_pos : queue) {
        m_n_leaves += count_leaves_from(root_node_pos);
    }

    // Number of nodes
    m_n_nodes_total = 0;
    while (not queue.empty()) {
        inf::EventTree::NodePos node_pos = util::pop_back(queue);

        ++m_n_nodes_total;

        if (node_pos.depth < get_depth() - 1) {
            add_children_to_queue(queue, node_pos);
        }
    }

    for (Index const depth : util::Range(m_depth)) {
        for (inf::EventTree::Node const &node : m_node_cache[depth]) {
            m_memory_footprint += node.get_memory_footprint();
        }
    }
}
