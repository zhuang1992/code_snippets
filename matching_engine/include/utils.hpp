#pragma once
#include <cstdlib>
#include <cstring>

#include "defs.hpp"

namespace orderbook {
inline std::string to_string(const sym_t& sym) {
    return std::string(sym.data());
}

inline bool convert_to_double(const std::string& str, double& val) {
    const char* c_str = str.c_str();
    char* endptr{};
    val = std::strtod(c_str, &endptr);
    return endptr == c_str + str.size();
}

inline bool convert_to_int(const std::string& str, int& val) {
    const char* c_str = str.c_str();
    char* endptr{};
    val = static_cast<int>(std::strtol(c_str, &endptr, 10));
    return endptr == c_str + str.size();
}

template <side_t side>
bool equal_or_more_aggresive(price_t p1, price_t p2) {
    if constexpr (side == BUY) {
        return p1 >= p2;
    } else {
        return p1 <= p2;
    }
}

// copied from cppreference
template <class To, class From>
std::enable_if_t<sizeof(To) == sizeof(From) &&
                     std::is_trivially_copyable_v<From> &&
                     std::is_trivially_copyable_v<To>,
    To>
bit_cast(const From& src) noexcept {
    static_assert(std::is_trivially_constructible_v<To>,
        "This implementation additionally requires "
        "destination type to be trivially constructible");

    To dst;
    std::memcpy(&dst, &src, sizeof(To));
    return dst;
}

struct book_key_hasher {
    std::size_t operator()(const sym_t& k) const {
        return bit_cast<uint64_t>(k);
    }
};
}  // namespace orderbook
