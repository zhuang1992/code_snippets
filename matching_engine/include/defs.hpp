#pragma once
#include <array>
namespace orderbook {
enum side_t { BUY, SELL, UNKNOWN };
static const uint8_t MAX_SYMBOL_LEN = 8;
static const uint32_t PRC_MULTIPLIER = 100000;
using sym_t = std::array<char,
    MAX_SYMBOL_LEN>;  // symbol will be padded with '\0' to MAX_SYMBOL_LEN
using qty_t = uint16_t;
using price_t = uint64_t;
}  // namespace orderbook
