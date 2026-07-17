#pragma once
#include <cmath>
#include <cstdio>

struct Vector {
    double x = 0;
    double y = 0;

    Vector() = default;
    Vector(double x_, double y_) : x(x_), y(y_) {}

    double operator~() const {
        return std::atan2(y, x) * 180.0 / 3.141592653589793 + 90.0;
    }

    bool operator==(const Vector& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Vector& o) const { return !(*this == o); }

    Vector operator+(const Vector& o) const { return {x + o.x, y + o.y}; }
    Vector operator-(const Vector& o) const { return {x - o.x, y - o.y}; }
    Vector operator*(double s) const { return {x * s, y * s}; }
    friend Vector operator*(double s, const Vector& v) { return v * s; }

    Vector& operator+=(const Vector& o) { x += o.x; y += o.y; return *this; }
    Vector& operator-=(const Vector& o) { x -= o.x; y -= o.y; return *this; }
    Vector& operator*=(double s) { x *= s; y *= s; return *this; }

    double length() const { return std::sqrt(x * x + y * y); }

    void print() const { std::printf("Vector(%g, %g)", x, y); }
};
