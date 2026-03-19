#include "bpt.h"
#include <iostream>

const int HEADER_SIZE = 1024;

BPT::BPT(const std::string& filename) : filename(filename), root_pos(-1), node_count(0), free_list(-1) {
    file.open(filename, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        // Create new file
        file.clear();
        file.open(filename, std::ios::out | std::ios::binary);
        file.close();
        file.open(filename, std::ios::in | std::ios::out | std::ios::binary);

        // Write header
        file.write(reinterpret_cast<const char*>(&root_pos), sizeof(root_pos));
        file.write(reinterpret_cast<const char*>(&node_count), sizeof(node_count));
        file.write(reinterpret_cast<const char*>(&free_list), sizeof(free_list));
        file.flush();
    } else {
        // Read header
        file.read(reinterpret_cast<char*>(&root_pos), sizeof(root_pos));
        file.read(reinterpret_cast<char*>(&node_count), sizeof(node_count));
        file.read(reinterpret_cast<char*>(&free_list), sizeof(free_list));
    }
}

BPT::~BPT() {
    if (file.is_open()) {
        // Update header
        file.seekp(0);
        file.write(reinterpret_cast<const char*>(&root_pos), sizeof(root_pos));
        file.write(reinterpret_cast<const char*>(&node_count), sizeof(node_count));
        file.write(reinterpret_cast<const char*>(&free_list), sizeof(free_list));
        file.flush();
        file.close();
    }
}

int BPT::allocate_node() {
    if (free_list != -1) {
        // Reuse a node from free list
        int pos = free_list;
        Node node;
        read_node(pos, node);
        free_list = node.next;
        return pos;
    } else {
        // Allocate new node
        int pos = HEADER_SIZE + node_count * sizeof(Node);
        node_count++;
        return pos;
    }
}

void BPT::deallocate_node(int pos) {
    Node node;
    node.next = free_list;
    free_list = pos;
    write_node(pos, node);
}

int BPT::read_node(int pos, Node& node) {
    if (pos < 0) return -1;
    file.seekg(pos);
    file.read(reinterpret_cast<char*>(&node), sizeof(Node));
    return file.gcount() == sizeof(Node) ? 0 : -1;
}

void BPT::write_node(int pos, const Node& node) {
    file.seekp(pos);
    file.write(reinterpret_cast<const char*>(&node), sizeof(Node));
    file.flush();
}

int BPT::find_leaf(const std::string& key) {
    if (root_pos == -1) return -1;

    Node node;
    int node_pos = root_pos;
    read_node(node_pos, node);

    while (!node.is_leaf) {
        int i = 0;
        while (i < node.key_count && strcmp(key.c_str(), node.keys[i].key) >= 0) {
            i++;
        }
        node_pos = node.children[i];
        read_node(node_pos, node);
    }

    return node_pos;
}

void BPT::insert(const std::string& key, int value) {
    KeyValue kv(key, value);

    if (root_pos == -1) {
        // Create root node
        root_pos = allocate_node();
        Node root;
        root.is_leaf = true;
        root.key_count = 1;
        root.keys[0] = kv;
        write_node(root_pos, root);
        return;
    }

    int leaf_pos = find_leaf(key);
    Node leaf;
    read_node(leaf_pos, leaf);

    // Check if key-value pair already exists
    for (int i = 0; i < leaf.key_count; i++) {
        if (strcmp(leaf.keys[i].key, key.c_str()) == 0 && leaf.keys[i].value == value) {
            return;  // Duplicate, don't insert
        }
    }

    insert_in_leaf(leaf_pos, kv);
}

void BPT::insert_in_leaf(int leaf_pos, const KeyValue& kv) {
    Node leaf;
    read_node(leaf_pos, leaf);

    // Find position to insert
    int i = leaf.key_count - 1;
    while (i >= 0 && leaf.keys[i] > kv) {
        leaf.keys[i + 1] = leaf.keys[i];
        i--;
    }
    leaf.keys[i + 1] = kv;
    leaf.key_count++;

    if (leaf.key_count <= MAX_KEYS) {
        write_node(leaf_pos, leaf);
    } else {
        // Split the leaf
        write_node(leaf_pos, leaf);
        split_leaf(leaf_pos);
    }
}

void BPT::split_leaf(int leaf_pos) {
    Node leaf, new_leaf;
    read_node(leaf_pos, leaf);

    // Create new leaf
    int new_leaf_pos = allocate_node();
    new_leaf.is_leaf = true;
    new_leaf.parent = leaf.parent;
    new_leaf.key_count = MIN_KEYS;

    // Copy half the keys to new leaf
    for (int i = 0; i < MIN_KEYS; i++) {
        new_leaf.keys[i] = leaf.keys[i + MIN_KEYS];
    }

    // Update original leaf
    leaf.key_count = MIN_KEYS;

    // Update linked list
    new_leaf.next = leaf.next;
    new_leaf.prev = leaf_pos;
    if (leaf.next != -1) {
        Node next_node;
        read_node(leaf.next, next_node);
        next_node.prev = new_leaf_pos;
        write_node(leaf.next, next_node);
    }
    leaf.next = new_leaf_pos;

    write_node(leaf_pos, leaf);
    write_node(new_leaf_pos, new_leaf);

    // Insert separator key in parent
    KeyValue separator = new_leaf.keys[0];
    insert_in_parent(leaf_pos, separator, new_leaf_pos);
}

void BPT::insert_in_parent(int left_pos, const KeyValue& kv, int right_pos) {
    if (left_pos == root_pos) {
        // Create new root
        int new_root_pos = allocate_node();
        Node new_root;
        new_root.is_leaf = false;
        new_root.key_count = 1;
        new_root.keys[0] = kv;
        new_root.children[0] = left_pos;
        new_root.children[1] = right_pos;
        write_node(new_root_pos, new_root);

        // Update children
        Node left, right;
        read_node(left_pos, left);
        read_node(right_pos, right);
        left.parent = new_root_pos;
        right.parent = new_root_pos;
        write_node(left_pos, left);
        write_node(right_pos, right);

        root_pos = new_root_pos;
        return;
    }

    Node left, parent;
    read_node(left_pos, left);
    read_node(left.parent, parent);

    // Find position in parent
    int i = 0;
    while (i < parent.key_count && parent.children[i] != left_pos) {
        i++;
    }

    // Shift keys and children
    for (int j = parent.key_count; j > i; j--) {
        parent.keys[j] = parent.keys[j - 1];
        parent.children[j + 1] = parent.children[j];
    }

    parent.key_count++;
    parent.keys[i] = kv;
    parent.children[i + 1] = right_pos;

    // Update right child's parent
    Node right;
    read_node(right_pos, right);
    right.parent = left.parent;
    write_node(right_pos, right);

    if (parent.key_count <= MAX_KEYS) {
        write_node(left.parent, parent);
    } else {
        // Split internal node
        write_node(left.parent, parent);
        split_internal(left.parent);
    }
}

void BPT::split_internal(int internal_pos) {
    Node internal, new_internal;
    read_node(internal_pos, internal);

    // Create new internal node
    int new_internal_pos = allocate_node();
    new_internal.is_leaf = false;
    new_internal.parent = internal.parent;
    new_internal.key_count = MIN_KEYS;

    // Copy half the keys and children
    for (int i = 0; i < MIN_KEYS; i++) {
        new_internal.keys[i] = internal.keys[i + MIN_KEYS + 1];
        new_internal.children[i] = internal.children[i + MIN_KEYS + 1];

        // Update children's parent
        if (new_internal.children[i] != -1) {
            Node child;
            read_node(new_internal.children[i], child);
            child.parent = new_internal_pos;
            write_node(new_internal.children[i], child);
        }
    }
    new_internal.children[MIN_KEYS] = internal.children[MAX_KEYS + 1];
    if (new_internal.children[MIN_KEYS] != -1) {
        Node child;
        read_node(new_internal.children[MIN_KEYS], child);
        child.parent = new_internal_pos;
        write_node(new_internal.children[MIN_KEYS], child);
    }

    // Update original internal
    internal.key_count = MIN_KEYS;

    write_node(internal_pos, internal);
    write_node(new_internal_pos, new_internal);

    // Insert separator key in parent
    KeyValue separator = internal.keys[MIN_KEYS];
    insert_in_parent(internal_pos, separator, new_internal_pos);
}

std::vector<int> BPT::find(const std::string& key) {
    std::vector<int> values;

    if (root_pos == -1) {
        return values;
    }

    int leaf_pos = find_leaf(key);
    Node leaf;
    read_node(leaf_pos, leaf);

    while (leaf_pos != -1) {
        bool found = false;
        for (int i = 0; i < leaf.key_count; i++) {
            if (strcmp(leaf.keys[i].key, key.c_str()) == 0) {
                values.push_back(leaf.keys[i].value);
                found = true;
            } else if (found) {
                // Since keys are sorted, we can stop when key changes
                return values;
            }
        }

        if (!found && leaf.key_count > 0 && strcmp(leaf.keys[leaf.key_count - 1].key, key.c_str()) < 0) {
            // Check next leaf
            leaf_pos = leaf.next;
            if (leaf_pos != -1) {
                read_node(leaf_pos, leaf);
            }
        } else {
            break;
        }
    }

    return values;
}

void BPT::remove(const std::string& key, int value) {
    KeyValue kv(key, value);

    if (root_pos == -1) {
        return;
    }

    // Find the leaf containing the key-value pair
    Node node;
    read_node(root_pos, node);

    int node_pos = root_pos;
    while (!node.is_leaf) {
        int i = 0;
        while (i < node.key_count && strcmp(key.c_str(), node.keys[i].key) >= 0) {
            i++;
        }
        node_pos = node.children[i];
        read_node(node_pos, node);
    }

    // Check if key-value pair exists
    bool found = false;
    for (int i = 0; i < node.key_count; i++) {
        if (strcmp(node.keys[i].key, key.c_str()) == 0 && node.keys[i].value == value) {
            found = true;
            break;
        }
    }

    if (!found) {
        return;  // Key-value pair not found
    }

    delete_entry(node_pos, kv);
}

void BPT::delete_entry(int node_pos, const KeyValue& kv) {
    Node node;
    read_node(node_pos, node);

    if (node.is_leaf) {
        remove_from_leaf(node_pos, kv);
    } else {
        remove_from_internal(node_pos, kv);
    }
}

void BPT::remove_from_leaf(int leaf_pos, const KeyValue& kv) {
    Node leaf;
    read_node(leaf_pos, leaf);

    // Find and remove the key-value pair
    int i = 0;
    while (i < leaf.key_count && !(leaf.keys[i] == kv)) {
        i++;
    }

    if (i >= leaf.key_count) return;  // Not found

    // Shift keys
    for (int j = i; j < leaf.key_count - 1; j++) {
        leaf.keys[j] = leaf.keys[j + 1];
    }
    leaf.key_count--;

    if (leaf_pos == root_pos) {
        // If root becomes empty, tree is empty
        if (leaf.key_count == 0) {
            deallocate_node(root_pos);
            root_pos = -1;
        } else {
            write_node(root_pos, leaf);
        }
    } else if (leaf.key_count < MIN_KEYS) {
        write_node(leaf_pos, leaf);

        // Handle underflow
        Node parent;
        read_node(leaf.parent, parent);

        int left_sib_pos = -1, right_sib_pos = -1;
        int left_sib_index = -1, right_sib_index = -1;

        // Find siblings
        for (int j = 0; j <= parent.key_count; j++) {
            if (parent.children[j] == leaf_pos) {
                if (j > 0) {
                    left_sib_pos = parent.children[j - 1];
                    left_sib_index = j - 1;
                }
                if (j < parent.key_count) {
                    right_sib_pos = parent.children[j + 1];
                    right_sib_index = j;
                }
                break;
            }
        }

        // Try redistribution first
        if (left_sib_pos != -1) {
            Node left_sib;
            read_node(left_sib_pos, left_sib);
            if (left_sib.key_count > MIN_KEYS) {
                redistribute_leaves(left_sib_pos, leaf_pos);
                return;
            }
        }

        if (right_sib_pos != -1) {
            Node right_sib;
            read_node(right_sib_pos, right_sib);
            if (right_sib.key_count > MIN_KEYS) {
                redistribute_leaves(leaf_pos, right_sib_pos);
                return;
            }
        }

        // Coalesce if redistribution not possible
        if (left_sib_pos != -1) {
            coalesce_leaves(left_sib_pos, leaf_pos);
        } else if (right_sib_pos != -1) {
            coalesce_leaves(leaf_pos, right_sib_pos);
        }
    } else {
        write_node(leaf_pos, leaf);
    }
}

void BPT::remove_from_internal(int, const KeyValue&) {
    // This function is not used in this implementation
    // since we only delete from leaves
}

void BPT::redistribute_leaves(int left_pos, int right_pos) {
    Node left, right, parent;
    read_node(left_pos, left);
    read_node(right_pos, right);
    read_node(left.parent, parent);  // Both have same parent

    // Find the index of the separator key in parent
    int sep_index = 0;
    while (sep_index < parent.key_count && parent.children[sep_index] != left_pos) {
        sep_index++;
    }

    if (left.key_count < MIN_KEYS && right.key_count > MIN_KEYS) {
        // Move a key from right to left
        left.keys[left.key_count] = right.keys[0];
        left.key_count++;

        // Shift keys in right
        for (int i = 0; i < right.key_count - 1; i++) {
            right.keys[i] = right.keys[i + 1];
        }
        right.key_count--;

        // Update separator key in parent
        parent.keys[sep_index] = right.keys[0];
    } else if (right.key_count < MIN_KEYS && left.key_count > MIN_KEYS) {
        // Move a key from left to right
        // Shift keys in right
        for (int i = right.key_count; i > 0; i--) {
            right.keys[i] = right.keys[i - 1];
        }

        // Move key from left to right
        right.keys[0] = left.keys[left.key_count - 1];
        right.key_count++;
        left.key_count--;

        // Update separator key in parent
        parent.keys[sep_index] = right.keys[0];
    }

    write_node(left_pos, left);
    write_node(right_pos, right);
    write_node(left.parent, parent);
}

void BPT::coalesce_leaves(int left_pos, int right_pos) {
    Node left, right, parent;
    read_node(left_pos, left);
    read_node(right_pos, right);
    read_node(left.parent, parent);

    // Find the separator key index in parent
    int sep_index = 0;
    while (sep_index < parent.key_count && parent.children[sep_index] != left_pos) {
        sep_index++;
    }

    // Move all keys from right to left
    for (int i = 0; i < right.key_count; i++) {
        left.keys[left.key_count + i] = right.keys[i];
    }
    left.key_count += right.key_count;

    // Update linked list
    left.next = right.next;
    if (right.next != -1) {
        Node next_node;
        read_node(right.next, next_node);
        next_node.prev = left_pos;
        write_node(right.next, next_node);
    }

    write_node(left_pos, left);
    deallocate_node(right_pos);

    // Remove separator key from parent
    for (int i = sep_index; i < parent.key_count - 1; i++) {
        parent.keys[i] = parent.keys[i + 1];
        parent.children[i + 1] = parent.children[i + 2];
    }
    parent.key_count--;

    if (parent.key_count == 0 && left.parent == root_pos) {
        // Root becomes empty, make left child the new root
        deallocate_node(root_pos);
        root_pos = left_pos;
        left.parent = -1;
        write_node(left_pos, left);
    } else {
        write_node(left.parent, parent);
    }
}