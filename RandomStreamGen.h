#pragma once
#include <string>
#include <random>
#include <sstream>
#include <vector>

class RandomStreamGen {
  size_t stream_size_ = 1000000;
  size_t step_ = 5;  // per cent
  size_t steps_left_ = 20;
  size_t part_size_ = 50000;
  size_t generated_count_ = 0;
  std::mt19937 rng = std::mt19937(std::random_device{}());
  std::uniform_int_distribution<int> size_dist = std::uniform_int_distribution<int>(1, 30);
  std::uniform_int_distribution<int> char_dist = std::uniform_int_distribution<int>(0, 62);

  static constexpr char symbols[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-";
  std::string generate();

 public:
  RandomStreamGen() = default;
  RandomStreamGen(int size, int step);
  ~RandomStreamGen() = default;
  bool isDried() const;
  std::vector<std::string> next();
};