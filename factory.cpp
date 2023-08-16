#include <memory>
#include <string>
#include <iostream>
#include <unordered_map>

// Base class
class Vehicle {
public:
    virtual ~Vehicle() = default;
    virtual void drive() = 0;
};

// Factory
class VehicleFactory {
private:
    using vehicle_ctor_fn = std::function<std::unique_ptr<Vehicle>(const std::string&)>;
    using map_t = std::unordered_map<std::string, vehicle_ctor_fn>;
    static map_t& get_map() {
        static map_t create_fn_map;
        return create_fn_map;
    }
    static void add_create_fn(const std::string& type, vehicle_ctor_fn fn) {
        auto& m = get_map();

        if (const auto& ret = m.emplace(type, fn); !ret.second) {
            throw std::runtime_error(std::string("Fail to register type: ") + type);
        }
    }
public:
    struct add_type {
        add_type(const std::string& type, vehicle_ctor_fn fn) {
            add_create_fn(type, fn);
        }
    };
    
    static std::unique_ptr<Vehicle> createVehicle(const std::string& type, const std::string& desc) {
        auto& vehicles = get_map();
        auto it = vehicles.find(type);
        if (it == vehicles.end()) {
            throw std::runtime_error("Invalid vehicle type");
        }
        auto& fn = it->second;
        return fn(desc);
    }
};

// Derived classes
class Car : public Vehicle {
    static inline VehicleFactory::add_type reg{"Car", [](const std::string& desc) { return std::make_unique<Car>(desc); }};
public:
    Car(const std::string& desc) {
        std::cout << "Creating a car with description: " << desc << '\n';
    }
    void drive() override {
        std::cout << "Driving a car\n";
    }
};

class Bike : public Vehicle {
    static inline VehicleFactory::add_type reg{"Bike", [](const std::string& desc) { return std::make_unique<Bike>(desc); }};
public:
    Bike(const std::string& desc) {
        std::cout << "Creating a bike with description: " << desc << '\n';
    }
    void drive() override {
        std::cout << "Riding a bike\n";
    }
};

// Test
int main() {
    auto car = VehicleFactory::createVehicle("Car", "blue");
    car->drive(); // Prints "Driving a car"

    auto bike = VehicleFactory::createVehicle("Bike", "red");
    bike->drive(); // Prints "Riding a bike"

    return 0;
}
