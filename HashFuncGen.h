#pragma once
#include <functional>
#include <cstdint>
#include "MurmurHash3.h"

class HashFuncGen {
  uint32_t seed_ = 42;

 public:
  explicit HashFuncGen(uint32_t);
  HashFuncGen() = default;
  ~HashFuncGen() = default;

  [[nodiscard]] std::function<size_t(const std::string&)> generate() const;
};