#include <iostream>

template <typename>
class function;

template <typename Ret, typename... Args>
class function<Ret(Args...)> {
private:
    struct Base {
        virtual ~Base() {}
        virtual Ret operator()(Args&&...) = 0;
    };

    template <typename T>
    struct Wrapper : Base {
        T func;
        Wrapper(const T& f) : func(f) {}
        Ret operator()(Args&&... args) override {
            return func(args...);
        }
    };

    std::unique_ptr<Base> func_holder;  // polymorphism is needed
public:
    function() = default;

    template <typename T>
    function(T f) : func_holder(new Wrapper<T>(f)) {}  // does the dynamic allocation. captured data are memebers of struct T

    Ret operator()(Args&&... args) {
        if (!func_holder) {
            throw std::bad_function_call();
        }
        return (*func_holder)(std::forward<Args>(args)...);
    }
};
void execute(function<int(int, int)> f) {
    std::cout << f(1,2) << std::endl;;
}
int main() {
    int x = 20;
    auto f = [x](int a, int b) {return a+b+x;};
    execute(f);
}