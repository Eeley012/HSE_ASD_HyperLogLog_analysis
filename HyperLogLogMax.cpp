#include "HyperLogLogMax.h"

int HyperLogLogMax::get_rank(const size_t index) const {
  const size_t start = index * bit_per_rank_val;
  int val = 0;
  for (size_t i = 0; i < bit_per_rank_val; ++i) {
    if (rank_[start + i])
      val |= (1 << i);
  }
  return val;
}

void HyperLogLogMax::set_rank(const size_t index, const int val) {
  const size_t start = index * bit_per_rank_val;
  for (size_t i = 0; i < bit_per_rank_val; ++i) {
    rank_[start + i] = (val >> i) & 1;
  }
}

HyperLogLogMax::HyperLogLogMax(std::function<size_t(const std::string&)> hasher) : hasher_(std::move(hasher)) {
  constexpr size_t pow2 = 1ULL << B_;
  constexpr double alpha = 0.7213 / (1.0 + 1.079 / static_cast<double>(pow2));
  ;
  nominator_ = static_cast<double>(pow2) * static_cast<double>(pow2) * alpha;
}
void HyperLogLogMax::process(const std::string& particle) {
  const uint32_t hash = hasher_(particle);
  const uint32_t index = hash >> (32 - B_);
  int rank_value;
  if (hash << B_ == 0) {
    rank_value = 32 - static_cast<int>(B_) + 1;
  } else {
    rank_value = 0;
    uint32_t temp = hash << B_;
    while ((temp & 0x80000000) == 0) {
      rank_value++;
      temp <<= 1;
    }
    rank_value++;
  }
  if (rank_value > get_rank(index)) {
    set_rank(index, rank_value);
  }
}

double HyperLogLogMax::evaluate() const {
  double denominator = 0.0;
  int empty = 0;

  for (size_t i = 0; i < rank_len_; ++i) {
    const int r = get_rank(i);
    denominator += std::pow(2.0, -r);
    if (r == 0) {
      empty++;
    }
  }

  double F = nominator_ / denominator;
  if (empty != 0 && F < rank_len_ * 2.5) {
    F = rank_len_ * std::log(rank_len_ / empty);
  }
  return F;
}