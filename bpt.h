#ifndef BPT_H
#define BPT_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstring>

const int MAX_KEYS = 128;  // Maximum keys per node
const int MIN_KEYS = 64;   // Minimum keys per node (except root)
const int VALUE_SIZE = 64; // Maximum size of index string

struct KeyValue {
    char key[VALUE_SIZE];
    int value;

    KeyValue() {
        memset(key, 0, VALUE_SIZE);
        value = 0;
    }

    KeyValue(const std::string& k, int v) {
        memset(key, 0, VALUE_SIZE);
        strncpy(key, k.c_str(), VALUE_SIZE - 1);
        value = v;
    }

    bool operator<(const KeyValue& other) const {
        int cmp = strcmp(key, other.key);
        if (cmp == 0) return value < other.value;
        return cmp < 0;
    }

    bool operator>(const KeyValue& other) const {
        int cmp = strcmp(key, other.key);
        if (cmp == 0) return value > other.value;
        return cmp > 0;
    }

    bool operator==(const KeyValue& other) const {
        return strcmp(key, other.key) == 0 && value == other.value;
    }
};

struct Node {
    bool is_leaf;
    int key_count;
    int parent;
    int next;  // For leaf nodes, points to next leaf
    int prev;  // For leaf nodes, points to previous leaf
    KeyValue keys[MAX_KEYS + 2];  // Extra space for split operations
    int children[MAX_KEYS + 2];   // For internal nodes

    Node() : is_leaf(true), key_count(0), parent(-1), next(-1), prev(-1) {
        memset(children, -1, sizeof(children));
    }
};

class BPT {
private:
    std::string filename;
    std::fstream file;
    int root_pos;
    int node_count;
    int free_list;  // Position of first free node

    int allocate_node();
    void deallocate_node(int pos);
    int read_node(int pos, Node& node);
    void write_node(int pos, const Node& node);
    int find_leaf(const std::string& key);
    void insert_in_leaf(int leaf_pos, const KeyValue& kv);
    void insert_in_parent(int left_pos, const KeyValue& kv, int right_pos);
    void split_leaf(int leaf_pos);
    void split_internal(int internal_pos);
    void delete_entry(int node_pos, const KeyValue& kv);
    void remove_from_leaf(int leaf_pos, const KeyValue& kv);
    void remove_from_internal(int internal_pos, const KeyValue& kv);
    void redistribute_leaves(int left_pos, int right_pos);
    void redistribute_internals(int left_pos, int right_pos);
    void coalesce_leaves(int left_pos, int right_pos);
    void coalesce_internals(int left_pos, int right_pos);

public:
    BPT(const std::string& filename);
    ~BPT();

    void insert(const std::string& key, int value);
    void remove(const std::string& key, int value);
    std::vector<int> find(const std::string& key);
};

#endif // BPT_H