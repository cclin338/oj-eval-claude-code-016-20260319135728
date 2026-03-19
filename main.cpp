#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <algorithm>
#include <cstring>

class SimpleBPT {
private:
    std::string filename;
    std::map<std::string, std::vector<int>> data;

    void loadFromFile() {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) return;

        int count;
        file.read(reinterpret_cast<char*>(&count), sizeof(count));

        for (int i = 0; i < count; i++) {
            char key[64];
            int val_count;
            file.read(key, 64);
            file.read(reinterpret_cast<char*>(&val_count), sizeof(val_count));

            std::vector<int> values(val_count);
            for (int j = 0; j < val_count; j++) {
                file.read(reinterpret_cast<char*>(&values[j]), sizeof(int));
            }

            data[std::string(key)] = values;
        }
    }

    void saveToFile() const {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return;

        int count = data.size();
        file.write(reinterpret_cast<const char*>(&count), sizeof(count));

        for (const auto& pair : data) {
            char key[64] = {0};
            strncpy(key, pair.first.c_str(), 63);
            file.write(key, 64);

            int val_count = pair.second.size();
            file.write(reinterpret_cast<const char*>(&val_count), sizeof(val_count));

            for (int val : pair.second) {
                file.write(reinterpret_cast<const char*>(&val), sizeof(int));
            }
        }
    }

public:
    SimpleBPT(const std::string& filename) : filename(filename) {
        loadFromFile();
    }

    ~SimpleBPT() {
        saveToFile();
    }

    void insert(const std::string& key, int value) {
        auto& values = data[key];
        // Check if value already exists for this key
        if (std::find(values.begin(), values.end(), value) == values.end()) {
            // Insert in sorted order
            values.insert(std::upper_bound(values.begin(), values.end(), value), value);
        }
    }

    void remove(const std::string& key, int value) {
        auto it = data.find(key);
        if (it != data.end()) {
            auto& values = it->second;
            auto val_it = std::find(values.begin(), values.end(), value);
            if (val_it != values.end()) {
                values.erase(val_it);
            }
            if (values.empty()) {
                data.erase(it);
            }
        }
    }

    std::vector<int> find(const std::string& key) {
        auto it = data.find(key);
        if (it != data.end()) {
            return it->second;
        }
        return std::vector<int>();
    }
};

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    SimpleBPT tree("database.db");

    int n;
    std::cin >> n;
    std::cin.ignore();  // Consume newline

    for (int i = 0; i < n; i++) {
        std::string line;
        std::getline(std::cin, line);
        std::istringstream iss(line);

        std::string command;
        iss >> command;

        if (command == "insert") {
            std::string key;
            int value;
            iss >> key >> value;
            tree.insert(key, value);
        } else if (command == "delete") {
            std::string key;
            int value;
            iss >> key >> value;
            tree.remove(key, value);
        } else if (command == "find") {
            std::string key;
            iss >> key;
            std::vector<int> values = tree.find(key);

            if (values.empty()) {
                std::cout << "null\n";
            } else {
                for (size_t j = 0; j < values.size(); j++) {
                    if (j > 0) std::cout << " ";
                    std::cout << values[j];
                }
                std::cout << "\n";
            }
        }
    }

    return 0;
}