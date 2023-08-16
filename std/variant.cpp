// #include <variant>
#include <iostream>
#include <algorithm>
#include <type_traits>
#include <typeinfo>

// traditional, recusive way to deal with variadic template args
template <size_t s1, size_t... others>
struct static_max;

template <size_t s>
struct static_max<s> {
	static const size_t value = s;
};

template <size_t s1, size_t s2, size_t... others>
struct static_max<s1, s2, others...> {
	static const size_t value = s1 >= s2 ? static_max<s1, others...>::value : static_max<s2, others...>::value;
};

// C++ 17 folding expression
template <size_t ... sizes>
constexpr std::size_t static_max_2() {
	std:: size_t ret{0};
	return ((ret = (sizes > ret ? sizes : ret)), ...);
};
template <size_t ... sizes>
constexpr std::size_t static_max_3() {
	std:: size_t ret{0};
	return ((ret=std::max( sizes, ret )), ...);
};
template <size_t ... sizes>
constexpr std::size_t static_max_4() {
	return std::max({sizes...});
}

template<typename T>
inline static void destroy(size_t id, void* data) {
	if (id == typeid(T).hash_code())
		reinterpret_cast<T*>(data)->~T();
}

template<typename T>
inline static void variant_clone(const size_t old_t, const void* old_v, void* new_v) {  // const is needed, because it's being used by copy ctor, which expect const args
	if (old_t == typeid(T).hash_code()) {
		new (new_v) T(*reinterpret_cast<const T*>(old_v));
	}
}

template<typename T>
inline static void variant_move(size_t old_t, void* old_v, void* new_v) {
	if (old_t == typeid(T).hash_code()) {
		new (new_v) T(std::move(*reinterpret_cast<T*>(old_v)));
	}
}


template<typename... Ts>
struct variant {
private:
	static const size_t data_size = static_max_4<sizeof(Ts)...>();
	// static const size_t data_size = static_max<sizeof(Ts)...>::value;
	static const size_t data_align = static_max<alignof(Ts)...>::value;
	using data_t = typename std::aligned_storage<data_size, data_align>::type;

	static inline size_t invalid_type() {
		return typeid(void).hash_code();
	}

	size_t type_id;
	data_t data;
public:
	variant() : type_id(invalid_type()) {}

	variant(const variant<Ts...>& o) : type_id(o.type_id) {
		((variant_clone<Ts>(o.type_id, &o.data, &data)), ...);
	}

	variant(variant<Ts...>&& o) : type_id(o.type_id) {
		((variant_move<Ts>(o.type_id, &o.data, &data)), ...);
	}
	variant<Ts...>& operator=(const variant<Ts...>& o) {
		((variant_clone<Ts>(o.type_id, &o.data, &data)), ...);
		return *this;
	}
	variant<Ts...>& operator=(variant<Ts...>&& o) {
		((variant_move<Ts>(o.type_id, &o.data, &data)), ...);
		return *this;
	}
	// variant<Ts...>& operator=(variant<Ts...> o){
	// 	std::swap(type_id, o.type_id);
	// 	std::swap(data, o.data);
	// 	return *this;
	// }
	
	template<typename T, typename...Args>
	void set(Args&&... args) {
		(destroy<Ts>(type_id, &data), ...);
		new (&data)T(std::forward<Args>(args)...);
		type_id = typeid(T).hash_code();
	}

	template<typename T>
	T& get() {
		if (type_id == typeid(T).hash_code())
			return *reinterpret_cast<T*>(&data);
		else
			throw std::bad_cast();
	}

	template<typename T>
	void is() {
		return (type_id == typeid(T).hash_code());
	}

	void valid() {
		return (type_id != invalid_type());
	}

	~variant() {
		(destroy<Ts>(type_id, &data), ...);
	}
};

struct test {
	int* holder{};
	test() {
		std::cout << "test()" << std::endl;
		holder = new int();
	}

	test(test&& old) : holder(nullptr) {
		std::cout << "test(test&&)" << std::endl;
		std::swap(holder, old.holder);
	}
	test(const test& old) {
		std::cout << "test(const test&)" << std::endl;
		holder = new int(*old.holder);
	}
	~test()
	{
		std::cout << "~test()" << std::endl;
		delete holder;
	}
};

int main() {
	using my_var = variant<std::string, test>;

	my_var d;

	d.set<std::string>("First string");
	std::cout << d.get<std::string>() << std::endl;

	d.set<test>();
	*d.get<test>().holder = 42;

	my_var e(std::move(d));
	std::cout << *e.get<test>().holder << std::endl;

	*e.get<test>().holder = 43;

	d = e;
	std::cout << *d.get<test>().holder << std::endl;

	d.set<std::string>("Second string");
	std::cout << d.get<std::string>() << std::endl;

}