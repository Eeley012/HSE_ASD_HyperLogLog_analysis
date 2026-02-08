#pragma once
#include <functional>
#include <string>
#include <vector>
#include <cmath>
#include <stdexcept>
#include <algorithm>

class HyperLogLog {
  std::function<size_t(const std::string&)> hasher_;
  size_t b_;
  std::vector<int> rank_;
  double nominator_;

 public:
  HyperLogLog() = delete;
  HyperLogLog(std::function<size_t(const std::string&)>, int);
  ~HyperLogLog() = default;
  void process(const std::string&);
  [[nodiscard]] double evaluate() const;
};