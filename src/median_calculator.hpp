#pragma once

#if defined(USE_BOOST) && USE_BOOST
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/median.hpp>

/**
 * Калькулятор медианы на базе Boost.Accumulators.
 * Позволяет инкрементально добавлять значения и извлекать медиану.
 */
class MedianCalculator {
public:
    MedianCalculator() = default;
    // Добавить новое значение в накопитель
    void add(double value) { acc(value); }
    // Получить текущую медиану. Если данных нет — возвращаем 0.0
    double median() const {
        using namespace boost::accumulators;
        if (n(acc) == 0) return 0.0;
        return extract::median(acc);
    }
private:
    boost::accumulators::accumulator_set<double, boost::accumulators::stats<boost::accumulators::tag::median> > acc;
};
#else
#include <queue>
#include <vector>

/**
 * Фоллбек-реализация калькулятора медианы, если Boost не доступен.
 * Реализована через две кучи (max-heap для нижней половины и min-heap для верхней).
 */
class MedianCalculator {
public:
    // Добавляем значение и балансируем кучи
    void add(double value) {
        if (low.empty() || value <= low.top()) low.push(value);
        else high.push(value);
        balance();
    }
    // Возвращаем медиану (0.0 при отсутствии данных)
    double median() const {
        if (low.empty() && high.empty()) return 0.0;
        if (low.size() == high.size()) return (low.top() + high.top()) / 2.0;
        return low.size() > high.size() ? low.top() : high.top();
    }
private:
    // Балансируем размеры куч, чтобы разница не превышала 1
    void balance() {
        if (low.size() > high.size() + 1) { high.push(low.top()); low.pop(); }
        else if (high.size() > low.size() + 1) { low.push(high.top()); high.pop(); }
    }
    // Нижняя половина — максимум сверху
    std::priority_queue<double> low;
    // Верхняя половина — минимум сверху
    std::priority_queue<double, std::vector<double>, std::greater<double>> high;
};
#endif
