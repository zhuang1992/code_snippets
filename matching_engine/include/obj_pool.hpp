#pragma once

#include <algorithm>

template <typename T, size_t Alignment = std::alignment_of_v<T>>
class obj_pool final {
    static constexpr size_t align_sz = std::max(Alignment, sizeof(int));
    struct obj_deleter {
        obj_pool* op_ = nullptr;

        explicit obj_deleter(obj_pool* op) : op_{op} {}
        void operator()(T* t) const { op_->nuke(t); }
    };

    obj_deleter od_;
    std::vector<T*> free_objs_;
    size_t allocated_objs_ = 0;

    /// pre-allocate uninitialized object memorys and store them in pool
    void grow(size_t sz = batch_count) {
        for (size_t i = 0; i < sz; ++i) {
            auto* b = malloc(obj_sz);
            free_objs_.push_back(static_cast<T*>(b));
        }
        allocated_objs_ += sz;
    }

    /// Call object destructor and release the memory to pool
    void nuke(T* t) {
        if (t) {
            t->~T();
            free_objs_.push_back(t);
        }
    }
    static constexpr size_t PAGE_SIZE = 4096;

    /// Round an unsigned integer up to a multiple of some other integer
    static inline constexpr size_t round_to_mult(size_t v, uint64_t multiple) {
        return static_cast<size_t>((v + multiple - 1) / multiple * multiple);
    }

 public:
    using unique_ptr = std::unique_ptr<T, obj_deleter>;
    static constexpr size_t obj_sz = round_to_mult(sizeof(T), align_sz);
    static_assert(obj_sz < PAGE_SIZE, "Object too large for obj_pool");

    static constexpr size_t batch_count = 128;

    obj_pool(obj_pool&&) = default;
    obj_pool(const obj_pool&) = delete;
    obj_pool& operator=(obj_pool&&) = delete;
    obj_pool& operator=(const obj_pool&) = delete;
    obj_pool() : od_{this} {}
    ~obj_pool() {
        for(auto* mem: free_objs_) {
            std::free(mem);
        }
    }

    /// Return number of object memory chunks being allocated
    size_t size() const { return allocated_objs_; }

    /// Return number of free object memory chunks can be used
    size_t free_size() const { return free_objs_.size(); }

    /// Request number of free object memory chunks not less than sz
    void reserve(size_t sz) {
        auto fsz = free_objs_.size();
        if (SLOW_PATH(sz <= fsz))
            return;
        auto rsz = round_to_mult(sz - fsz, batch_count);
        grow(rsz);
    }

    template <typename... Args>
    unique_ptr make(Args&&... args) {
        if (free_objs_.empty())
            grow();

        auto* p = free_objs_.back();
        free_objs_.pop_back();
        return unique_ptr(new (p) T{std::forward<Args>(args)...}, od_);
    }
};
