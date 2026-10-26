#pragma once
#include <stdint.h>
#include <array>

class Color final{
private :

	std::array<uint8_t, 4> c;

public:




    Color(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 0) : c({ r, g, b, a }) {}
    Color(const std::array<uint8_t, 4>& color) : c(color) {}
    Color(uint32_t color): c({ static_cast<uint8_t>(color & 0xFF), static_cast<uint8_t>((color >> 8) & 0xFF), static_cast<uint8_t>((color >> 16) & 0xFF), static_cast<uint8_t>((color >> 24) & 0xFF)}){}

    Color operator+(const Color& other) const;
    Color& operator=(const Color& color) = default;

    inline uint8_t r() const { return c[0]; }
    inline uint8_t g() const { return c[1]; }
    inline uint8_t b() const { return c[2]; }
    inline uint8_t a() const { return c[3]; }


    uint32_t Getrgb() const ;
    void setRgb(uint8_t a, uint8_t r, uint8_t g, uint8_t b);



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