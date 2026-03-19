#include <iostream>
#include <sstream>
#include <vector>
#include "bpt.h"

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    BPT tree("database.db");

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
                std::sort(values.begin(), values.end());
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