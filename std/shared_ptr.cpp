#include <atomic>
#include <iostream>
#include <vector>

template<typename T>
class shared_ptr {
private:
    T* ptr{};  // pointer to the managed object
    std::atomic<size_t>* count{};  // reference count

public:
    // constructor
    explicit shared_ptr(T* p=nullptr) : ptr(p), count(new std::atomic<size_t>(1)) {}
    shared_ptr(const T& d): ptr(new T(d)), count(new std::atomic<size_t>(1)) {}

    // copy constructor
    shared_ptr(const shared_ptr& other) : ptr(other.ptr), count(other.count) {
        count->fetch_add(1);
    }

    // copy assignment operator
    shared_ptr& operator=(const shared_ptr& other) {
        if (this != &other) {
            this->~shared_ptr();  // destroy current object
            ptr = other.ptr;
            count = other.count;
            count->fetch_add(1);
        }
        return *this;
    }

    shared_ptr(shared_ptr&& o) noexcept {
        // no need for self assignment check for constructor, because there is no self yet
        ptr = o.ptr;
        count = o.count;
        o.ptr = nullptr;
        o.count = nullptr;
    }
    shared_ptr& operator=(shared_ptr&& o) noexcept {
        if (this == &o) {
            return;
        }
        this->~shared_ptr();
        ptr = o.ptr;
        count = o.count;
        o.ptr = nullptr;
        o.count = nullptr;
        return *this;
    }

    // destructor
    ~shared_ptr() {
        if (count && count->fetch_sub(1) == 1) {  // check whether count is nullptr first
            delete ptr;
            delete count;
        }
    }

    // dereference operator
    T& operator*() const {
        return *ptr;
    }

    // member access operator
    T* operator->() const {
        return ptr;
    }
};
template<typename T, typename... Args>
shared_ptr<T> make_shared(Args&&... args) {
    return shared_ptr<T>(new T(std::forward<Args>(args)...));
}
struct Foo {
    void say() const {
        std::cout << "Foo says hi!\n";
    }
};

int main() {
    shared_ptr<Foo> p1(new Foo);
    {
        shared_ptr<Foo> p2 = p1;
        p2->say();
    }
    p1->say();

    auto p2 = make_shared<Foo>();
    p2->say();

    std::vector<shared_ptr<Foo>> vec;
    vec.push_back(make_shared<Foo>());
    // vec.push_back(new Foo());
    vec.emplace_back(new Foo());
    return 0;
}
