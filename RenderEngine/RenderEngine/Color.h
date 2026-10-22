#pragma once
#include <stdint.h>
#include <array>

class Color final{
private :

	std::array<uint8_t, 4> _c;

public:




    Color(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 255) : _c({ r, g, b, a }) {}
    Color(const std::array<uint8_t, 4>& color) : _c(color) {}

    Color& operator=(const Color& color) = default;

    inline uint8_t r() const { return _c[0]; }
    inline uint8_t g() const { return _c[1]; }
    inline uint8_t b() const { return _c[2]; }
    inline uint8_t a() const { return _c[3]; }


    static const Color WHITE;
    static const Color BLACK;
    static const Color GRAY;
    static const Color WHITE_GRAY;
    static const Color RED;
    static const Color GREEN;
    static const Color BLUE;
    static const Color YELLOW;
    static const Color LIGHT_YELLOW;
    static const Color LIGHT_ORANGE;


};