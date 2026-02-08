#include <iostream>
#include "RandomStreamGen.h"
#include "HashFuncGen.h"
#include "HyperLogLog.h"
#include "HyperLogLogMax.h"

#include <fstream>
#include <unordered_set>

// собираем данные для определения оптимального параметра B
// для скромного использования памяти и приемлемой точности
void b_value_selection() {
  auto sg = RandomStreamGen();
  const auto hfg = HashFuncGen();
  // Cоздаём разные алгоритмы HLL с b = 2, 4, 6, .. 16
  std::vector<HyperLogLog> hll;
  hll.reserve(20);
  for (auto i = 0; i < 9; i++) {
    hll.emplace_back(hfg.generate(), (i + 1) * 2);
  }
  // Создаём точный определитель
  std::unordered_set<std::string> set;
  // Скармливаем 1 и тот же поток данных разным hll и точному set
  std::cout << "\nB selection started\n" << std::endl;
  while (!sg.isDried()) {
    std::vector<std::string> part = sg.next();
    for (const auto& s : part) {
      for (auto i = 0; i < 9; i++) {
        hll[i].process(s);
      }
      set.insert(s);
    }
  }
  // точная оценка
  size_t precise = set.size();
  // собираем данные
  std::ofstream csv("b_selection2.csv");
  csv << "b,mem_bytes,exact,hll_res,error_percent\n";
  for (int i = 0; i < 9; i++) {
    // результаты наших HLL
    double hll_res = hll[i].evaluate();
    // относительная ошибка
    double error = std::abs(hll_res - static_cast<double>(set.size())) / static_cast<double>(set.size()) * 100.0;
    // память в байтах
    int b = (i + 1) * 2;
    size_t memory = (1 << b) * 4;

    csv << b << "," << memory << "," << precise << "," << hll_res << "," << error << "\n";
  }
  std::cout << "B selection ended\n\n\n" << std::endl;
}

// собираем данные для визуализации погрешности работы HLL относительно
// точного подсчета на протяжении одного потока (график 1)
//
// и для визуализации статистического среднего отклонения
// по данным серии прогонов (график 2)
void evaluation_statistics() {
  int streams_total = 30;
  std::vector<std::vector<double>> all_streams_data(streams_total);
  for (int i = 0; i < streams_total; i++) {
    all_streams_data[i].reserve(31000);  // планирую 30К, но с запасом
  }
  std::vector<size_t> precise;
  precise.reserve(110);  // должно быть 100 частей потока и для каждой - точная оценка

  // логи на случай долгой работы
  std::cout << "There will be " << streams_total << " runs with b=" << 10 << "...\n";

  // данные отклонения от точного количества - только на первом прогоне
  std::unordered_set<std::string> set;

  for (int i = 0; i < streams_total; ++i) {
    std::cout << "Run " << i + 1 << "/" << streams_total << "..." << std::flush;

    // каждый прогон - новый поток
    RandomStreamGen sg(3000000, 1);  // 3M частиц в потоке, шаг - 1% от количества частиц
    // каждый прогон - новый хешер
    HashFuncGen hfg(42 * i);
    // каждый прогон - новый HLL
    HyperLogLog hll(hfg.generate(), 10);  // HLL генерируются с b = 10 (подобрано эмпирически)

    while (!sg.isDried()) {
      std::vector<std::string> part = sg.next();
      for (const auto& s : part) {
        hll.process(s);
        if (i == 0) {
          set.insert(s);
        }
      }
      all_streams_data[i].push_back(hll.evaluate());
      if (i == 0) {
        precise.push_back(set.size());
      }
    }
    std::cout << "Run finished...\n" << std::endl;
  }

  // записываем в csv
  std::ofstream csv("data.csv");
  csv << "step,precise";
  for (int i = 0; i < streams_total; ++i) {
    csv << ",run_" << i;
  }
  csv << "\n";

  for (size_t j = 0; j < all_streams_data[0].size(); ++j) {
    size_t current_step_count = (j + 1) * 10000;
    csv << current_step_count << "," << precise[j];

    for (int i = 0; i < streams_total; ++i) {
      csv << "," << std::fixed << std::setprecision(2) << all_streams_data[i][j];
    }
    csv << "\n";
  }

  csv.close();
  std::cout << "Completed successfully.\n";
}

// подсчет дисперсии
void precision_data() {
  // оптимальная константа b = 10 и неудачная b = 5
  // std::vector<int> test_b_values = {5, 10};
  std::vector<int> test_b_values = {10};

  std::ofstream csv("dispersion_data.csv");
  // параметр b, аналитическая погрешность, эмпирическая погрешност
  csv << "b,sigma_anl_104,sigma_anl_130,sigma_emp\n";

  for (int b : test_b_values) {
    std::cout << "\nB = " << b << "\n\n" << std::flush;
    std::vector<double> errors;
    errors.reserve(50);

    for (int i = 0; i < 50; ++i) {
      if (i % 5 == 0) {
        std::cout << "Run " << i + 1 << "\n" << std::flush;
      }
      RandomStreamGen sg(3000000, 5);
      HashFuncGen hfg(42 + i * 11);
      HyperLogLog hll(hfg.generate(), b);

      std::unordered_set<std::string> set;
      size_t precise = 0;

      while (!sg.isDried()) {
        auto part = sg.next();
        for (const auto& s : part) {
          hll.process(s);
          set.insert(s);
        }
      }
      precise = set.size();
      double hll_res = hll.evaluate();

      double rel_error = (hll_res - static_cast<double>(precise)) / static_cast<double>(precise);
      errors.push_back(rel_error);
    }

    // подсчет дисперсии и сигмы
    double sum = 0;
    for (auto error : errors) {
      sum += error;
    }
    double mean_error = sum / 50;

    double sq_sum = 0.0;
    for (double err : errors) {
      double diff = err - mean_error;
      sq_sum += diff * diff;
    }
    double variance = sq_sum / (50 - 1);
    double sigma_exp_pct = std::sqrt(variance) * 100.0;

    // аналитическое значение сигмы
    auto m = static_cast<double>(1 << b);
    double sigma_anl_104 = (1.04 / std::sqrt(m)) * 100.0;
    double sigma_anl_130 = (1.30 / std::sqrt(m)) * 100.0;

    csv << b << "," << sigma_anl_104 << "," << sigma_anl_130 << "," << sigma_exp_pct << "\n";
  }
  csv.close();
}

// собираем данные для определения оптимального параметра B
// сравнивая vec и bitset
void b_value_selection_vec_bit() {
  auto sg = RandomStreamGen(10000000, 1);
  const auto hfg = HashFuncGen();
  // Cоздаём разные алгоритмы HLL с b = 2, 4, 6, .. 16
  std::vector<HyperLogLog> hll;
  hll.reserve(20);
  for (auto i = 0; i < 9; i++) {
    hll.emplace_back(hfg.generate(), (i + 1) * 2);
  }
  // Создаём точный определитель
  std::unordered_set<std::string> set;
  // Скармливаем 1 и тот же поток данных разным hll и точному set
  std::cout << "\nB selection started\n" << std::endl;
  while (!sg.isDried()) {
    std::vector<std::string> part = sg.next();
    for (const auto& s : part) {
      for (auto i = 0; i < 9; i++) {
        hll[i].process(s);
      }
      set.insert(s);
    }
  }
  // точная оценка
  size_t precise = set.size();
  // собираем данные
  std::ofstream csv("b_selection_vec_bit.csv");
  csv << "b,memory_vec,memory_bitset,exact,hll_res,error_percent\n";
  for (int i = 0; i < 9; i++) {
    // результаты наших HLL
    double hll_res = hll[i].evaluate();
    // относительная ошибка
    double error = std::abs(hll_res - static_cast<double>(set.size())) / static_cast<double>(set.size()) * 100.0;
    // память в байтах
    int b = (i + 1) * 2;
    size_t memory_vec = (1 << b) * 4;
    size_t memory_bitset = ((1 << b) * 6) / 8;

    csv << b << "," << memory_vec << "," << memory_bitset << "," << precise << "," << hll_res << "," << error << "\n";
  }
  std::cout << "B selection ended\n\n\n" << std::endl;
}

// улучшенный HLL
void evaluation_and_precision_upgraded() {
  int streams_total = 30;
  std::vector<std::vector<double>> all_streams_data(streams_total);
  for (int i = 0; i < streams_total; i++) {
    all_streams_data[i].reserve(31000);  // планирую 30К, но с запасом
  }
  std::vector<size_t> precise;
  precise.reserve(110);  // должно быть 100 частей потока и для каждой - точная оценка

  // логи на случай долгой работы
  std::cout << "There will be " << streams_total << " runs with b=" << 14 << "...\n";

  // данные отклонения от точного количества - только на первом прогоне
  std::unordered_set<std::string> set;

  for (int i = 0; i < streams_total; ++i) {
    std::cout << "Run " << i + 1 << "/" << streams_total << "..." << std::flush;

    // каждый прогон - новый поток
    RandomStreamGen sg(3000000, 1);  // 3M частиц в потоке, шаг - 1% от количества частиц
    // каждый прогон - новый хешер
    HashFuncGen hfg(42 * i);
    // каждый прогон - новый HLL
    HyperLogLogMax hll(hfg.generate());  // HLL генерируются с b = 12 (подобрано эмпирически)

    while (!sg.isDried()) {
      std::vector<std::string> part = sg.next();
      for (const auto& s : part) {
        hll.process(s);
        if (i == 0) {
          set.insert(s);
        }
      }
      all_streams_data[i].push_back(hll.evaluate());
      if (i == 0) {
        precise.push_back(set.size());
      }
    }
    std::cout << "Run finished...\n" << std::endl;
  }

  // записываем в csv
  std::ofstream csv("data.csv");
  csv << "step,precise";
  for (int i = 0; i < streams_total; ++i) {
    csv << ",run_" << i;
  }
  csv << "\n";

  for (size_t j = 0; j < all_streams_data[0].size(); ++j) {
    size_t current_step_count = (j + 1) * 10000;
    csv << current_step_count << "," << precise[j];

    for (int i = 0; i < streams_total; ++i) {
      csv << "," << std::fixed << std::setprecision(2) << all_streams_data[i][j];
    }
    csv << "\n";
  }
  csv.close();
  std::cout << "Completed successfully.\n";
  //__________________________________________________________
  // оптимальная константа b = 14
  std::vector<int> test_b_values = {14};

  std::ofstream csv1("dispersion_data.csv");
  // параметр b, аналитическая погрешность, эмпирическая погрешност
  csv1 << "b,sigma_anl_104,sigma_anl_130,sigma_emp\n";

  for (int b : test_b_values) {
    std::cout << "\nB = " << b << "\n\n" << std::flush;
    std::vector<double> errors;
    errors.reserve(50);

    for (int i = 0; i < 50; ++i) {
      if (i % 5 == 0) {
        std::cout << "Run " << i + 1 << "\n" << std::flush;
      }
      RandomStreamGen sg(3000000, 1);
      HashFuncGen hfg(42 + i * 11);
      HyperLogLogMax hll(hfg.generate());

      std::unordered_set<std::string> set;
      size_t precise = 0;

      while (!sg.isDried()) {
        auto part = sg.next();
        for (const auto& s : part) {
          hll.process(s);
          set.insert(s);
        }
      }
      precise = set.size();
      double hll_res = hll.evaluate();

      double rel_error = (hll_res - static_cast<double>(precise)) / static_cast<double>(precise);
      errors.push_back(rel_error);
    }

    // подсчет дисперсии и сигмы
    double sum = 0;
    for (auto error : errors) {
      sum += error;
    }
    double mean_error = sum / 50;

    double sq_sum = 0.0;
    for (double err : errors) {
      double diff = err - mean_error;
      sq_sum += diff * diff;
    }
    double variance = sq_sum / (50 - 1);
    double sigma_exp_pct = std::sqrt(variance) * 100.0;

    // аналитическое значение сигмы
    auto m = static_cast<double>(1 << b);
    double sigma_anl_104 = (1.04 / std::sqrt(m)) * 100.0;
    double sigma_anl_130 = (1.30 / std::sqrt(m)) * 100.0;

    csv1 << b << "," << sigma_anl_104 << "," << sigma_anl_130 << "," << sigma_exp_pct << "\n";
  }
  csv1.close();
}

int main() {
  // b_value_selection();
  evaluation_statistics();
  // precision_data();
  // b_value_selection_vec_bit();
  evaluation_and_precision_upgraded();
  return 0;
}
