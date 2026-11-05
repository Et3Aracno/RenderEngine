#include "Color.h"
#include <algorithm>

Color Color::operator+(const Color& other) const
{
    // saturation à 255 : sans ça, uint8_t déborde et la couleur "tourne"
    return Color(
        (uint8_t)std::min(255, r() + other.r()),
        (uint8_t)std::min(255, g() + other.g()),
        (uint8_t)std::min(255, b() + other.b()),
        (uint8_t)std::min(255, a() + other.a())
    );
}

uint32_t Color::getRgb() const
{
    return ((uint32_t)a() << 24) | ((uint32_t)b() << 16)
        | ((uint32_t)g() << 8) | (uint32_t)r();
}

void Color::setRgb(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    c_[0] = r;
    c_[1] = g;
    c_[2] = b;
    c_[3] = a;
}

uint32_t Color::colorScale(uint32_t c, float k)
{
    if (k < 0.0f) k = 0.0f;
    if (k > 1.0f) k = 1.0f;

    const uint8_t sk = static_cast<uint8_t>(k);
    uint32_t r = static_cast<uint32_t>((c >> 16) * sk);
    uint32_t g = static_cast<uint32_t>((c >> 8) * sk);
    uint32_t b = static_cast<uint32_t>((c * sk));
    return ((uint32_t)0xFFu << 24) | (r << 16) | (g << 8) | b;
}

// Ordre des arguments : (r, g, b, a)
const Color Color::white = Color(255, 255, 255, 255);
const Color Color::black = Color(0, 0, 0, 255);
const Color Color::gray = Color(100, 100, 100, 255);
const Color Color::whiteGray = Color(200, 200, 200, 255);
const Color Color::red = Color(255, 0, 0, 255);
const Color Color::green = Color(0, 255, 0, 255);
const Color Color::blue = Color(0, 0, 255, 255);
const Color Color::yellow = Color(255, 255, 0, 255);
const Color Color::lightYellow = Color(255, 255, 224, 255);
const Color Color::lightOrange = Color(255, 200, 170, 255);