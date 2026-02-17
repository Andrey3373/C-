#pragma once

#include <string>
#include <vector>

/**
 * Структура конфигурации приложения.
 * - input_dir: директория с входными CSV
 * - output_dir: директория для результата
 * - filename_mask: список масок для фильтрации файлов
 */
struct AppConfig {
    std::string input_dir;
    std::string output_dir;
    std::vector<std::string> filename_mask;
};

/**
 * Читает и валидирует TOML-конфиг по указанному пути.
 * В случае ошибок функция должна бросать исключение с описанием проблемы.
 */
AppConfig parse_config(const std::string &path);
