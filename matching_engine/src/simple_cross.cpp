
#include <fstream>
#include <iostream>
#include <list>

#include "orderbook.hpp"

namespace orderbook {

class SimpleCross {
 public:
    results_t action(const std::string& line) {
        log.clear();
        parse_and_execute(line);
        return log.retrieve_data();
    }

 private:
    OrderBookMgr obm_;

    static auto split_str_by_delim(const std::string& str, const char delim) {
        std::vector<std::string> result;
        size_t start = 0;
        auto end = str.find(delim);
        while (end != std::string::npos) {
            result.push_back(str.substr(start, end - start));
            start = end + 1;
            end = str.find(delim, start);
        }
        result.push_back(str.substr(start));
        return result;
    }

    void parse_and_execute(const std::string& line) {
        auto tokens = split_str_by_delim(line, ' ');
        if (tokens.empty())
            return;
        if (tokens[0] == "O") {
            if (tokens.size() != 6) {
                log.add_err(tokens[1] + " Invalid number of arguments");
                return;
            }
            auto side = tokens[3] == "B" ? side_t::BUY
                                         : (tokens[3] == "S" ? side_t::SELL
                                                             : side_t::UNKNOWN);
            if (side == UNKNOWN) {
                log.add_err(tokens[1] + " Invalid side: " + tokens[3]);
                return;
            }
            int32_t qty{};
            if (!convert_to_int(tokens[4], qty) || qty <= 0) {
                log.add_err(tokens[1] + " Invalid qty: " + tokens[4]);
                return;
            }
            double prc{};
            if (!convert_to_double(tokens[5], prc) || prc <= 0) {
                log.add_err(tokens[1] + " Invalid prc: " + tokens[5]);
                return;
            }

            auto oid = std::stoi(tokens[1]);
            sym_t symbol;
            symbol.fill(0);
            std::copy(tokens[2].begin(), tokens[2].end(), symbol.begin());

            obm_.add_order(static_cast<uint32_t>(oid), symbol, side,
                static_cast<qty_t>(qty),
                static_cast<price_t>(prc * PRC_MULTIPLIER));
        } else if (tokens[0] == "X") {
            if (tokens.size() != 2) {
                log.add_err(tokens[1] + " Invalid number of arguments");
                return;
            }
            auto oid = static_cast<uint32_t>(std::stoi(tokens[1]));
            obm_.cxl_order(oid);
        } else if (tokens[0] == "P") {
            obm_.print_books();
        } else {
            log.add_err(tokens[0] + " Invalid action");
        }
    }
};

}  // namespace orderbook

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Usage: ./simple_cross [input_file]" << std::endl;
        return 0;
    }
    orderbook::SimpleCross scross;
    std::string line;
    std::ifstream actions(argv[1], std::ios::in);
    while (std::getline(actions, line)) {
        orderbook::results_t results = scross.action(line);
        for (orderbook::results_t::const_iterator it = results.begin();
             it != results.end(); ++it) {
            std::cout << *it << std::endl;
        }
    }
    return 0;
}
