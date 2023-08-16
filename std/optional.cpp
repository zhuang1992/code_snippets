#include <type_traits>
#include <iostream>
#include <string>

template <typename T>
struct optional {
private:
	union { char dummy_; T data_; }; // alternatively, use std::aligned_storage_t, but it's not trivially destructible (POD)
	bool valid_{};

public:
	optional() : valid_{}, dummy_{0} {
		std::cout << "default ctor called" << std::endl;
	}
	optional(const optional<T>& other) {
		std::cout << "copy ctor called" << std::endl;
		valid_ = other.valid_;
		// this won't work for union	ATTENTION: we cannot do data_=other.data_; because T may have constructors and this doesn't work for Union (eg. string)
		// data_ = other.data; 
		new (&data_) T(other.data_);  // uses copy ctor
	}
	optional<T>& operator=(const optional<T>& other) {
		std::cout << "copy assignment called" << std::endl;
		if (this == &other)
			return *this;
		if (valid_) {
			data_.~T();
			valid_ = false;
		}
		new (&data_) T(other.data_);
		valid_ = other.valid_;
		return *this;
	}
	optional(optional<T>&& other) noexcept {
		std::cout << "move ctor called" << std::endl;
		valid_ = other.valid_;
		new (&data_) T(std::move(other.data_));
		other.valid_ = false;
	}
	optional<T>& operator=(optional<T>&& other) noexcept {
		std::cout << "move assignment called" << std::endl;
		if (this == &other)
			return *this;
		if (valid_) {
			data_.~T();
			valid_ = false;
		}
		valid_ = other.valid_;
		new (&data_) T(std::move(other.data_));
		other.valid_ = false;
		return *this;
	}

	// from T
	optional(const T& data): valid_(true) {
		std::cout << "ctor from const T& called" << std::endl;
		new (&data_) T(data);
	}
	optional(T&& data) : valid_(true) {
		std::cout << "ctor from T&& called" << std::endl;
		new (&data_)T(std::move(data));
	}
	
	optional& operator=(const T& data) {
		if (&data == &data_) {
			return *this;
		}
		if (valid_) {
			data_.~T();
			valid_ = false;
		}
		new (&data_) T(data);
		valid_ = true;
		std::cout << "operator=(const T&) called" << std::endl;
		return *this;
	}
	
	bool has_value() { return valid_; }
	T& get() { 
		if (!valid_)
			throw std::runtime_error("accessing invalid data");
		return data_; 
	}
	~optional() {
		if (valid_)
			data_.~T();
	}
};

int main() {
	optional<std::string> test;
	std::cout << test.has_value() << std::endl;
	std::string x("aqbc");
	test = x;
	test = "abc";
	std::cout << test.has_value() << ',' << test.get() << std::endl;

	auto test2 = test;
	std::cout << test2.has_value() << ',' << test2.get() << std::endl;

	optional<std::string> test3("xxx");
	std::cout << test3.has_value() << ',' << test3.get() << std::endl;

	optional<std::string> test4(x);
	std::cout << test4.has_value() << ',' << test4.get() << std::endl;

	auto test5 = std::move(test4);
	std::cout << test5.has_value() << ',' << test5.get() << std::endl;

	optional<std::string> test6("ababa");
	test6 = std::move(test5);
	std::cout << test6.has_value() << ',' << test6.get() << std::endl;
}