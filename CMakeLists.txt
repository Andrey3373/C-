cmake_minimum_required(VERSION 3.23)
project(csv_median_calculator VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# ---
# Простые комментарии для разработчика (на русском):
# Этот CMakeLists подтягивает минимальные зависимости через FetchContent,
# предпочитая встроенные заголовки для toml++ (в папке third_party).
# ---

# Use only spdlog via FetchContent to avoid heavy external dependencies
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.12.0
)
FetchContent_MakeAvailable(spdlog)

# toml++ via archive URL to avoid git tag checkout issues

# toml++ подключается локально: ожидаем, что исходники лежат в third_party/tomlplusplus
add_library(tomlplusplus INTERFACE)
target_include_directories(tomlplusplus INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tomlplusplus/include)
add_library(tomlplusplus::tomlplusplus ALIAS tomlplusplus)

find_package(Boost 1.74 COMPONENTS program_options)
if (Boost_FOUND)
  # Если найден Boost — будем использовать Boost.Program_options и Boost.Accumulators
  message(STATUS "Found Boost: ${Boost_INCLUDE_DIRS}")
else()
  # Если Boost отсутствует — проект соберётся с ограниченной функциональностью (фоллбеки в коде)
  message(WARNING "Boost not found: falling back to internal parsers. To use Boost, install Boost and re-run CMake.")
endif()

add_executable(csv_median_calculator
  src/main.cpp
  src/config_parser.cpp
  src/csv_reader.hpp
  src/median_calculator.hpp
)

target_link_libraries(csv_median_calculator PRIVATE
  tomlplusplus::tomlplusplus
  spdlog::spdlog
)

if (Boost_FOUND)
  target_link_libraries(csv_median_calculator PRIVATE Boost::program_options)
  target_compile_definitions(csv_median_calculator PRIVATE USE_BOOST=1)
else()
  target_compile_definitions(csv_median_calculator PRIVATE USE_BOOST=0)
endif()

target_compile_features(csv_median_calculator PRIVATE cxx_std_23)

install(TARGETS csv_median_calculator RUNTIME DESTINATION bin)

# Конец CMakeLists
