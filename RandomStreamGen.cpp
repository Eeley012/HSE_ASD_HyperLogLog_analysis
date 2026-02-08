#include "RandomStreamGen.h"

std::string RandomStreamGen::generate() {
  const int len = size_dist(rng);
  std::string particle;
  particle.reserve(len);
  for (int i = 0; i < len; ++i) {
    particle += symbols[char_dist(rng)];
  }
  return particle;
}

std::vector<std::string> RandomStreamGen::next() {
  if (steps_left_ == 1) {
    part_size_ = stream_size_ - generated_count_;
  }
  std::vector<std::string> part;
  part.reserve(part_size_);

  for (size_t i = 0; i < part_size_; ++i) {
    part.push_back(generate());
  }
  generated_count_ += part_size_;
  steps_left_--;
  return part;
}

bool RandomStreamGen::isDried() const {
  return steps_left_ == 0;
}

RandomStreamGen::RandomStreamGen(const int size, const int step) {
  stream_size_ = size;
  step_ = step;
  steps_left_ = 100 / step_;
  part_size_ = stream_size_ / steps_left_;
  generated_count_ = 0;
}
