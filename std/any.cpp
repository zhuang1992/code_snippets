#include <memory>
#include <typeinfo>

class any {
private:
    struct Base {
        virtual ~Base() {}
        virtual std::unique_ptr<Base> clone() const = 0;
        virtual const std::type_info& type() const = 0;
    };

    template<typename T>
    struct Derived : Base {
        template<typename U>
        Derived(U&& value) : value(std::forward<U>(value)) {}

        std::unique_ptr<Base> clone() const override {
            return std::make_unique<Derived<T>>(value);
        }

        const std::type_info& type() const override {
            return typeid(T);
        }

        T value;
    };
    std::unique_ptr<Base> ptr;

public:
    any() = default;

    // without this condition, infinite recursion will happen when an any object is constructed with another any object
    template<typename T, typename = typename std::enable_if_t<!std::is_same_v<typename std::decay_t<T>, any>>>
    any(T&& value) : ptr(std::make_unique<Derived<std::decay_t<T>>>(std::forward<T>(value))) {}

    // copy constructor
    any(const any& other) : ptr(other.ptr ? other.ptr->clone() : nullptr) {}
    any& operator=(const any& other) {
        if (ptr == other.ptr)
            return *this;
        ptr = other.ptr ? other.ptr->clone() : nullptr;
        return *this;
    }

    // move constructor
    any(any&&) noexcept = default;
    any& operator=(any&&) noexcept = default;

    bool has_value() const {
        return bool(ptr);
    }

    const std::type_info& type() const {
        return has_value() ? ptr->type() : typeid(void);
    }

    template<typename T>
    T& as() {
        if(typeid(T) != type())
            throw std::bad_cast();
        return static_cast<Derived<T>*>(ptr.get())->value;
    }

    template<typename T>
    const T& as() const {
        if(typeid(T) != type())
            throw std::bad_cast();
        return static_cast<Derived<T>*>(ptr.get())->value;
    }
};

#include <iostream>
#include <string>

int main() {
    any a = 5;
    std::cout << "int: " << a.as<int>() << '\n';

    a = std::string("Hello, world!");
    std::cout << "string: " << a.as<std::string>() << '\n';

    a = 1.234;
    std::cout << "double: " << a.as<double>() << '\n';

    try {
        std::cout << a.as<int>() << '\n';  // throws std::bad_cast
    } catch (const std::bad_cast& e) {
        std::cout << e.what() << '\n';
    }

    any b = a;
    std::cout << "b double: " << b.as<double>() << '\n';


    return 0;
}
