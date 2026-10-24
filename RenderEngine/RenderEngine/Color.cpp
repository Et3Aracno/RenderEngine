#include "Color.h"


inline Color Color::operator+(const Color& other) const {
    return Color
    (
        r() + other.r(),
        g() + other.g(),
        b() + other.b(),
        a() + other.a()
    );
};

uint32_t Color::Getrgb() const {
    return ((uint32_t)a() << 24) | ((uint32_t)b() << 16)
        | ((uint32_t)g() << 8) | (uint32_t)r();
}

void Color::setRgb(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
    c[0] = r;
    c[1] = g;
    c[2] = b;
    c[3] = a;
}


 const Color Color::WHITE = Color(255, 255, 255, 255);
 const Color Color::BLACK = Color(255, 0, 0, 0);
 const Color Color::GRAY = Color(255, 100, 100, 100);
 const Color Color::WHITE_GRAY = Color(255 , 200, 200, 200);
 const Color Color::RED = Color(255, 255, 0, 0);
 const Color Color::GREEN = Color(255, 0, 255, 0);
 const Color Color::BLUE = Color(255, 0, 0, 255);
 const Color Color::YELLOW = Color(255, 255, 255, 0);
 const Color Color::LIGHT_YELLOW = Color(255, 255, 255, 224);
 const Color Color::LIGHT_ORANGE = Color(255, 255, 200, 170);