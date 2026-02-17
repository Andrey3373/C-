
# csv_median_calculator

Краткое описание: консольное приложение для расчёта инкрементальной медианы цен из CSV-файлов.

Подробное техническое задание и правила code-style находятся в `specs.md`.

Сборка:

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Запуск:

```
./build/csv_median_calculator -config config.toml
```

Файлы: `config.toml` пример, `examples/input` — примеры CSV.
