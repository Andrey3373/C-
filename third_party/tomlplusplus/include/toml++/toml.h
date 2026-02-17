// Minimal shim implementing a tiny subset of toml++ API used in this project.
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace toml {
    struct node {
        enum class Type { None, String, Array, Table } type = Type::None;
        std::string s;
        std::vector<node> arr;
        std::unordered_map<std::string, node> tbl;

        bool is_string() const { return type == Type::String; }
        template<typename T>
        T value_or(const T &def) const {
            if constexpr (std::is_same_v<T, std::string>) {
                if (type == Type::String) return s; else return def;
            }
            return def;
        }

        std::vector<node>* as_array() {
            if (type == Type::Array) return &arr; return nullptr;
        }
        node* as_table() {
            if (type == Type::Table) return this; return nullptr;
        }
        node& operator[](const std::string &k) { return tbl[k]; }
        bool contains(const std::string &k) const { return tbl.find(k) != tbl.end(); }
    };

    struct table : node {
        table() { type = Type::Table; }
    };

    // very small parser: only reads [main] section and simple keys
    inline table parse_file(const std::string &path) {
        table root;
        std::ifstream f(path);
        if (!f.is_open()) throw std::runtime_error("Failed to open toml file");
        std::string line;
        std::string cur_section;
        while (std::getline(f, line)) {
            // remove comments
            auto pos = line.find('#');
            if (pos != std::string::npos) line = line.substr(0, pos);
            // trim
            auto first = line.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) continue;
            auto last = line.find_last_not_of(" \t\r\n");
            std::string l = line.substr(first, last-first+1);
            if (l.empty()) continue;
            if (l.front() == '[' && l.back() == ']') {
                cur_section = l.substr(1, l.size()-2);
                continue;
            }
            auto eq = l.find('='); if (eq == std::string::npos) continue;
            std::string key = l.substr(0, eq);
            std::string val = l.substr(eq+1);
            // trim
            auto trim = [](std::string &s){
                auto a = s.find_first_not_of(" \t\r\n"); if (a==std::string::npos) { s.clear(); return; }
                auto b = s.find_last_not_of(" \t\r\n"); s = s.substr(a, b-a+1);
            };
            trim(key); trim(val);
            if (cur_section == "main") {
                if (key == "filename_mask") {
                    node arr; arr.type = node::Type::Array;
                    if (val.size()>=2 && val.front()=='[' && val.back()==']') {
                        std::string inner = val.substr(1, val.size()-2);
                        std::stringstream ss(inner);
                        std::string item;
                        while (std::getline(ss, item, ',')) {
                            trim(item);
                            if (!item.empty() && item.front()=='\'' && item.back()=='\'') item = item.substr(1, item.size()-2);
                            node n; n.type = node::Type::String; n.s = item; arr.arr.push_back(n);
                        }
                    }
                    root.tbl[cur_section].tbl[key] = arr;
                } else {
                    // strip surrounding quotes if present
                    if (!val.empty() && ((val.front()=='\"' && val.back()=='\"') || (val.front()=='\'' && val.back()=='\''))) {
                        val = val.substr(1, val.size()-2);
                    }
                    node n; n.type = node::Type::String; n.s = val;
                    root.tbl[cur_section].type = node::Type::Table;
                    root.tbl[cur_section].tbl[key] = n;
                }
            }
        }
        return root;
    }
}
