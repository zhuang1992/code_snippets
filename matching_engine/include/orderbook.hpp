#pragma once
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "obj_pool.hpp"
#include "output_collector.hpp"
#include "utils.hpp"

namespace orderbook {

struct Order;
class OrderBook;

static output_collector log;

struct PriceLevel {
    PriceLevel(price_t p, side_t s) : prc(p), side(s) {}
    price_t prc{};
    side_t side{};
    OrderBook* ob{};
    std::list<Order*> ords;
};
struct Order {
    Order(uint32_t id, qty_t q) : oid(id), qty(q) {}
    uint32_t oid{};
    qty_t qty{};
    PriceLevel* pl{};
    std::list<Order*>::iterator ord_it{};
    auto* book() const { return pl->ob; }
    auto side() const { return pl->side; }
};

class OrderBook {
 public:
    explicit OrderBook(sym_t symbol) : symbol_(symbol) {}

    template <side_t SIDE>
    void match_order(Order* ord, price_t prc,
        std::unordered_map<uint32_t, obj_pool<Order>::unique_ptr>& ord_map);

    template <side_t SIDE>
    void add_order(Order* ord, price_t prc);

    void remove_order(const Order* ord);

    void print_book();

 private:
    template <side_t SIDE, typename Levels>
    void do_add_order(Levels& lvls, Order* ord, price_t prc);
    template <side_t SIDE, typename Levels>
    void do_match_order(Levels& lvls, Order* ord, price_t prc,
        std::unordered_map<uint32_t, obj_pool<Order>::unique_ptr>& ord_map);

    sym_t symbol_{};
    obj_pool<PriceLevel> pl_pool_;
    // sorted from most aggressive to least aggressive
    std::map<price_t, obj_pool<PriceLevel>::unique_ptr, std::greater<>> bids_;
    std::map<price_t, obj_pool<PriceLevel>::unique_ptr> asks_;
};

class OrderBookMgr {
 public:
    void add_order(
        uint32_t oid, const sym_t& sym, side_t side, qty_t qty, price_t prc);
    void cxl_order(uint32_t oid);
    void print_books();

 private:
    obj_pool<Order> ord_pool_;
    std::unordered_map<sym_t, OrderBook, book_key_hasher> books_;
    std::unordered_map<uint32_t, obj_pool<Order>::unique_ptr> orders_;
};


template <side_t SIDE>
void OrderBook::match_order(Order* ord, price_t prc,
                 std::unordered_map<uint32_t, obj_pool<Order>::unique_ptr>& ord_map) {
    if constexpr (SIDE == BUY)
    do_match_order<SIDE>(asks_, ord, prc, ord_map);
    else
    do_match_order<SIDE>(bids_, ord, prc, ord_map);
}

template <side_t SIDE>
void OrderBook::add_order(Order* ord, price_t prc) {
    if constexpr (SIDE == BUY) {
        do_add_order<BUY>(bids_, ord, prc);
    } else {
        do_add_order<SELL>(asks_, ord, prc);
    }
}

void OrderBook::remove_order(const Order* ord) {
    // remove the order from price level
    ord->pl->ords.erase(ord->ord_it);

    // remove the price level if it is empty
    if (ord->pl->ords.empty()) {
        if (ord->side() == BUY) {
            bids_.erase(ord->pl->prc);
        } else {
            asks_.erase(ord->pl->prc);
        }
    }
}

void OrderBook::print_book() {
    // print ask side by price high->low
    for (auto it = asks_.rbegin(); it != asks_.rend(); ++it) {
        auto& [prc, pl] = *it;
        // print orders from latest to earliest
        for (auto ord_it = pl->ords.rbegin(); ord_it != pl->ords.rend();
             ++ord_it) {
            auto* ord = *ord_it;
            log.add_order(symbol_, ord->oid, SELL, ord->qty, prc);
        }
    }
    for (auto& [prc, pl] : bids_) {
        for (auto* ord : pl->ords) {
            log.add_order(symbol_, ord->oid, BUY, ord->qty, prc);
        }
    }
}
template <side_t SIDE, typename Levels>
void OrderBook::do_add_order(Levels& lvls, Order* ord, price_t prc) {
    auto it = lvls.find(prc);
    if (it == lvls.end()) {
        // create a new price level
        auto pl = pl_pool_.make(prc, SIDE);
        ord->pl = pl.get();
        pl->ords.push_back(ord);
        pl->ob = this;
        ord->ord_it = std::prev(pl->ords.end());
        lvls.emplace(prc, std::move(pl));
    } else {
        // add to existing price level
        auto& pl = it->second;
        ord->pl = pl.get();
        pl->ords.push_back(ord);
        ord->ord_it = std::prev(pl->ords.end());
    }
}
template <side_t SIDE, typename Levels>
void OrderBook::do_match_order(Levels& lvls, Order* ord, price_t prc,
                    std::unordered_map<uint32_t, obj_pool<Order>::unique_ptr>& ord_map) {
    while (ord->qty > 0 && !lvls.empty()) {
        auto it = lvls.begin();
        auto& pl = it->second;

        if (!equal_or_more_aggresive<SIDE>(prc, pl->prc)) {
            break;
        }

        auto& ords = pl->ords;
        auto& top_ord = ords.front();
        if (top_ord->qty > ord->qty) {
            // top order is larger than incoming order
            top_ord->qty -= ord->qty;
            log.add_fill(symbol_, ord->oid, ord->qty, pl->prc);
            log.add_fill(symbol_, top_ord->oid, ord->qty, pl->prc);
            ord->qty = 0;
        } else {
            // top order is smaller or equal to incoming order
            ord->qty -= top_ord->qty;
            log.add_fill(symbol_, ord->oid, top_ord->qty, pl->prc);
            log.add_fill(symbol_, top_ord->oid, top_ord->qty, pl->prc);
            top_ord->qty = 0;
            auto remove_oid = top_ord->oid;
            remove_order(top_ord);
            ord_map.erase(remove_oid);
        }
    }
}
void OrderBookMgr::add_order(
    uint32_t oid, const sym_t& sym, side_t side, qty_t qty, price_t prc) {
    auto [it, inserted] =
    orders_.try_emplace(oid, ord_pool_.make(oid, qty));
    if (!inserted) {
        log.add_err(std::to_string(oid) + " Duplicate order id");
        return;
    }
    auto [book_it, bb] = books_.try_emplace(sym, sym);
    auto& ord = it->second;
    auto& book = book_it->second;
    if (side == BUY) {
        book.match_order<BUY>(ord.get(), prc, orders_);
        if (ord->qty > 0)
            book.add_order<BUY>(ord.get(), prc);
    } else {
        book.match_order<SELL>(ord.get(), prc, orders_);
        if (ord->qty > 0)
            book.add_order<SELL>(ord.get(), prc);
    }
    if (ord->qty == 0) {
        orders_.erase(it);
    }
}
void OrderBookMgr::cxl_order(uint32_t oid) {
    auto it = orders_.find(oid);
    if (it == orders_.end()) {
        log.add_err(std::to_string(oid) + " Order not found");
        return;
    }
    auto& order = it->second;
    order->book()->remove_order(order.get());
    orders_.erase(it);
    log.add_cxl(oid);
}
void OrderBookMgr::print_books() {
    for (auto& [sym, book] : books_) {
        book.print_book();
    }
}
}  // namespace orderbook
