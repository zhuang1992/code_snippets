#pragma once
#include <iomanip>
#include <list>
#include <sstream>
#include <string>

#include "defs.hpp"
#include "utils.hpp"

namespace orderbook {

typedef std::list<std::string> results_t;
class output_collector {
 public:
    void add_fill(const sym_t& symbol, uint32_t oid, qty_t qty, price_t prc) {
        std::stringstream ss;
        ss << "F " << oid << " " << to_string(symbol) << " " << qty << " "
           << format_prc(prc);
        results_.push_back(ss.str());
    }
    void add_cxl(uint32_t oid) {
        std::stringstream ss;
        ss << "X " << oid;
        results_.push_back(ss.str());
    }
    void add_err(const std::string& msg) {
        std::stringstream ss;
        ss << "E " << msg;
        results_.push_back(ss.str());
    }
    void add_order(const sym_t& symbol, uint32_t oid, side_t side, qty_t qty,
        price_t prc) {
        std::stringstream ss;
        ss << "P " << oid << " " << to_string(symbol) << " "
           << (side == BUY ? 'B' : 'S') << " " << qty << " " << format_prc(prc);
        results_.push_back(ss.str());
    }
    auto retrieve_data() {
        results_t data;
        std::stringstream ss;
        ss << "results.size() == " << results_.size();
        int i = 0;
        for (auto& line : results_) {
            ss << "\n\tresults[" << i++ << "] == \"" << line << "\"";
        }
        data.push_back(ss.str());
        return data;
    }
    void clear() { results_.clear(); }

 private:
    static std::string format_prc(price_t p, int digits = 5) {
        const double prc_multiplier = 1. / PRC_MULTIPLIER;
        std::stringstream ss;
        ss << std::fixed << std::setprecision(digits) << p * prc_multiplier;
        return ss.str();
    }
    results_t results_;
};

}  // namespace orderbook
