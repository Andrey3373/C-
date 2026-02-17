#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <future>
#include <spdlog/spdlog.h>

/**
 * Описание записи, которую мы извлекаем из CSV.
 * Используем только необходимые поля: `receive_ts` и `price`.
 */
struct Record {
    uint64_t receive_ts;
    double price;
};

/**
 * Читает один CSV-файл и возвращает вектор `Record`.
 * 
 * Ожидается разделитель `;` и первая строка — заголовок.
 * В случае ошибки парсинга строки логируем и пропускаем её.
 */
static std::vector<Record> read_single_file(const std::filesystem::path &path) {
    std::vector<Record> out;
    std::ifstream f(path);
    if (!f.is_open()) { spdlog::error("Failed to open {}", path.string()); return out; }

    std::string line;
    bool header = true;
    size_t lineno = 0;
    while (std::getline(f, line)) {
        ++lineno;
        if (line.empty()) continue;
        if (header) { header = false; continue; }
        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> cols;
        while (std::getline(ss, token, ';')) cols.push_back(token);
        if (cols.size() < 3) {
            // Логируем ошибку формата и продолжаем обработку файла
            spdlog::error("Invalid CSV format {}:{}", path.string(), lineno);
            continue;
        }
        try {
            uint64_t ts = std::stoull(cols[0]);
            double price = std::stod(cols[2]);
            out.push_back(Record{ts, price});
        } catch (...) {
            // Ошибка преобразования числовых значений — логируем и пропускаем
            spdlog::error("Failed to parse numeric value {}:{}", path.string(), lineno);
        }
    }
    return out;
}

/**
 * Сканирует директорию `input_dir`, фильтрует файлы по маскам (если указаны)
 * и читает найденные CSV-файлы в параллельных задачах.
 * Возвращает объединённый вектор всех записей.
 */
std::vector<Record> read_csv_files(const std::string &input_dir, const std::vector<std::string> &masks) {
    std::vector<Record> out;
    namespace fs = std::filesystem;
    if (!fs::exists(input_dir)) {
        spdlog::error("Input directory does not exist: {}", input_dir);
        return out;
    }

    // Сбор файлов, соответствующих маскам и с расширением .csv
    std::vector<fs::path> files;
    for (auto &p : fs::directory_iterator(input_dir)) {
        if (!p.is_regular_file()) continue;
        auto path = p.path();
        if (path.extension() != ".csv") continue;
        std::string name = path.filename().string();
        if (!masks.empty()) {
            bool ok = false;
            for (auto &m : masks) if (!m.empty() && name.find(m) != std::string::npos) { ok = true; break; }
            if (!ok) continue;
        }
        files.push_back(path);
    }

    if (files.empty()) return out;

    // Запускаем параллельное чтение файлов (std::async)
    std::vector<std::future<std::vector<Record>>> futs;
    futs.reserve(files.size());
    for (auto &p : files) futs.push_back(std::async(std::launch::async, read_single_file, p));

    // Собираем результаты и объединяем в один вектор
    size_t total = 0;
    for (auto &f : futs) {
        try {
            auto vec = f.get();
            total += vec.size();
            // Перемещаем элементы во внешний вектор, чтобы избежать лишних копий
            out.insert(out.end(), std::make_move_iterator(vec.begin()), std::make_move_iterator(vec.end()));
        } catch (const std::exception &e) {
            spdlog::error("Error reading file task: {}", e.what());
        }
    }
    spdlog::info("Loaded {} records from {} files", total, files.size());
    return out;
}

