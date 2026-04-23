#ifndef UTIL_PRNG_HPP_
#define UTIL_PRNG_HPP_

#include <cstdint>
#include <random>

namespace ripple::util {

class PRNG {
private:
  std::random_device prng;
  std::mt19937 gen;

  // random bytes engine
  std::independent_bits_engine<std::mt19937, 32, uint32_t> rbe_u32;
  std::independent_bits_engine<std::mt19937, 64, uint64_t> rbe_u64;

public:
  std::uniform_real_distribution<> dist;

  PRNG() {
    gen = std::mt19937(prng());
    dist = std::uniform_real_distribution<>();
    rbe_u32 = std::independent_bits_engine<std::mt19937, 32, uint32_t>(gen);
    rbe_u64 = std::independent_bits_engine<std::mt19937, 64, uint64_t>(gen);
  };

  inline double random() { return dist(gen); };

  inline uint32_t random_u32() { return rbe_u32(); };
  inline uint64_t random_u64() { return rbe_u64(); };
};

} // namespace ripple::util

#endif /* UTIL_PRNG_HPP_*/