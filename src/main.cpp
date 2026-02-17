#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <iomanip>

#include <spdlog/spdlog.h>
#if defined(USE_BOOST) && USE_BOOST
#include <boost/program_options.hpp>
#endif

#include "config_parser.hpp"
#include "csv_reader.hpp"
#include "median_calculator.hpp"

namespace fs = std::filesystem;

/**
 * Основная точка входа приложения.
 * Программа читает конфиг, загружает CSV-записи, рассчитывает
 * инкрементальную медиану цен и записывает изменения в выходной CSV.
 */
int main(int argc, char **argv) {
    try {
        // Логируем запуск приложения
        spdlog::info("Запуск csv_median_calculator");
#if defined(USE_BOOST) && USE_BOOST
        namespace po = boost::program_options;
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help", "produce help message")
            ("config,cfg", po::value<std::string>(), "path to config.toml");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) { std::cout << desc << '\n'; return 0; }

        std::string cfg_path;
        if (vm.count("config") || vm.count("cfg")) cfg_path = vm.count("config") ? vm["config"].as<std::string>() : vm["cfg"].as<std::string>();
        else { cfg_path = "config.toml"; spdlog::info("Config not specified, using {}", cfg_path); }
#else
        std::string cfg_path;
        for (int i = 1; i < argc; ++i) {
            std::string a = argv[i];
            if (a == "-config" || a == "-cfg" || a == "--config") { if (i + 1 < argc) { cfg_path = argv[i+1]; break; } }
            if (a == "-h" || a == "--help" || a == "-help") { std::cout << "Usage: csv_median_calculator [-config path]" << std::endl; return 0; }
        }
        if (cfg_path.empty()) { cfg_path = "config.toml"; spdlog::info("Config not specified, using {}", cfg_path); }
#endif

        AppConfig cfg = parse_config(cfg_path);


        // Логируем пути из конфига
        spdlog::info("Input dir: {}", cfg.input_dir);
        spdlog::info("Output dir: {}", cfg.output_dir);

        // Читаем CSV-файлы и получаем вектор записей (receive_ts, price)
        auto records = read_csv_files(cfg.input_dir, cfg.filename_mask);
        spdlog::info("Read records: {}", records.size());

        std::sort(records.begin(), records.end(), [](auto &a, auto &b){
            return a.receive_ts < b.receive_ts;
        });

        // Создаём директорию вывода при необходимости и открываем файл результата
        fs::create_directories(cfg.output_dir);
        std::ofstream out(cfg.output_dir + "/median_result.csv");
        out << "receive_ts;price_median\n";

        // Калькулятор медианы (может использовать boost или фоллбек)
        MedianCalculator calc;
        // last_median хранит последнее записанное значение медианы.
        // Изначально NaN — значит ещё не было записей.
        double last_median = std::numeric_limits<double>::quiet_NaN();
        size_t changes = 0;

        // Проходим по всем записям в порядке receive_ts
        for (auto &r : records) {
            // Добавляем новую цену и вычисляем текущую медиану
            calc.add(r.price);
            double m = calc.median();
            // Сравнение с учётом числовой погрешности
            bool changed = std::isnan(last_median) || std::fabs(m - last_median) > 1e-9;
            if (changed) {
                // Записываем только при изменении медианы
                out << r.receive_ts << ";" << std::fixed << std::setprecision(8) << m << "\n";
                last_median = m;
                ++changes;
            }
        }

        spdlog::info("Median changes written: {}", changes);
        spdlog::info("Result saved to {}/median_result.csv", cfg.output_dir);
    } catch (const std::exception &e) {
        spdlog::error("Fatal error: {}", e.what());
        return 2;
    }
    return 0;
}
