#pragma once

#include <jex_registry.hpp>

namespace jex {

struct Complex {
    double real = 0.0;
    double imaginary = 0.0;

    Complex() = default;
    Complex(double real, double imaginary)
    : real(real)
    , imaginary(imaginary) {
    }

    Complex operator+(const Complex& other) const {
        return Complex(real + other.real, imaginary + other.imaginary);
    }
    bool operator==(const Complex& other) const {
        return real == other.real && imaginary == other.imaginary;
    }
};

static constexpr char ComplexName[] = "Complex";
using ArgComplex = ArgValue<Complex, ComplexName>;

class MathModule : public Module {
    void registerTypes(Registry& registry) const override;
    void registerFcts(Registry& registry) const override;
};

} // namespace jex
