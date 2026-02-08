#include "HashFuncGen.h"

HashFuncGen::HashFuncGen(const uint32_t seed) : seed_(seed) {
}

std::function<size_t(const std::string&)> HashFuncGen::generate() const {
  uint32_t captured_seed = seed_;
  return [captured_seed](const std::string& key) -> size_t {
    uint32_t out_hash = 0;
    MurmurHash3_x86_32(key.data(), static_cast<int>(key.size()), captured_seed, &out_hash);
    return out_hash;
  };
}
