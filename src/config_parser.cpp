#include "config_parser.hpp"
#include <toml++/toml.h>
#include <spdlog/spdlog.h>
#include <filesystem>

AppConfig parse_config(const std::string &path) {
    AppConfig cfg;
    namespace fs = std::filesystem;
    if (!fs::exists(path)) {
        spdlog::error("Config file not found: {}", path);
        throw std::runtime_error("Config not found");
    }

    try {
        auto tbl = toml::parse_file(path);
        if (!tbl.contains("main")) {
            spdlog::error("Config missing [main] table: {}", path);
            throw std::runtime_error("Missing [main] table");
        }
        auto main = tbl["main"];
        if (main.contains("input")) cfg.input_dir = main["input"].value_or<std::string>(".");
        if (main.contains("output")) cfg.output_dir = main["output"].value_or<std::string>("");
        if (main.contains("filename_mask")) {
            if (auto arr = main["filename_mask"].as_array()) {
                for (auto &v : *arr) {
                    if (v.is_string()) cfg.filename_mask.push_back(v.value_or<std::string>(""));
                }
            }
        }
        if (cfg.input_dir.empty()) {
            spdlog::error("Config: input directory is required");
            throw std::runtime_error("input required");
        }
        if (cfg.output_dir.empty()) {
            cfg.output_dir = fs::current_path().string() + "/output";
            spdlog::info("Output not set, using {}", cfg.output_dir);
        }
    } catch (const std::exception &e) {
        spdlog::error("TOML parse/open error: {}", e.what());
        throw;
    }

    return cfg;
}
