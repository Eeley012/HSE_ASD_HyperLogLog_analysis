#pragma once
#include <functional>
#include <string>
#include <vector>
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <bitset>

class HyperLogLogMax {
  static constexpr size_t B_ = 13;
  static constexpr size_t rank_len_ = 1 << B_;
  static constexpr size_t bit_per_rank_val = 6;

  std::function<size_t(const std::string&)> hasher_;
  std::bitset<rank_len_ * bit_per_rank_val> rank_{};
  double nominator_;

  [[nodiscard]] int get_rank(size_t index) const;
  void set_rank(size_t index, int val);

 public:
  HyperLogLogMax() = delete;
  explicit HyperLogLogMax(std::function<size_t(const std::string&)>);
  ~HyperLogLogMax() = default;
  void process(const std::string&);
  [[nodiscard]] double evaluate() const;
};