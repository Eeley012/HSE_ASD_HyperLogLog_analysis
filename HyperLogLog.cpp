#include "HyperLogLog.h"

HyperLogLog::HyperLogLog(std::function<size_t(const std::string&)> hasher, const int b) : hasher_(std::move(hasher)) {
  if (b < 2 || b > 30) {
    throw std::invalid_argument("b must be in [2, 30]");
  }
  b_ = b;
  const size_t pow2 = 1ULL << b;
  rank_ = std::vector<int>(pow2, 0);
  double alpha;
  switch (pow2) {
    case 16:
      alpha = 0.673;
      break;
    case 32:
      alpha = 0.697;
      break;
    case 64:
      alpha = 0.709;
      break;
    default:  // для m >= 128
      alpha = 0.7213 / (1.0 + 1.079 / static_cast<double>(pow2));
  }
  nominator_ = static_cast<double>(pow2) * static_cast<double>(pow2) * alpha;
}
void HyperLogLog::process(const std::string& particle) {
  const uint32_t hash = hasher_(particle);
  const uint32_t index = hash >> (32 - b_);
  int rank_value;
  if (hash << b_ == 0) {
    rank_value = 32 - static_cast<int>(b_) + 1;
  } else {
    rank_value = __builtin_clz(hash << b_) + 1;
  }
  rank_[index] = std::max(rank_[index], rank_value);
}

double HyperLogLog::evaluate() const {
  double denominator = 0.0;
  int empty = 0;

  for (const int r : rank_) {
    denominator += std::pow(2.0, -r);
    if (r == 0) {
      empty++;
    }
  }

  double F = nominator_ / denominator;
  const auto rank_len = static_cast<double>(rank_.size());
  if (empty != 0 && F < (1 << b_) * 2.5) {

    F = rank_len * std::log(rank_len / empty);
  }
  return F;
}