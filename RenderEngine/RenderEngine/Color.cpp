#include "Color.h"




 inline uint32_t Color::rgb(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
	return ((uint32_t)a << 24) | ((uint32_t)b << 16)
		| ((uint32_t)g << 8) | (uint32_t)r;
}


 const Color Color::WHITE = Color(255, 255, 255, 255);
 const Color Color::BLACK = Color(0, 0, 0);
 const Color Color::GRAY = Color(100, 100, 100);
 const Color Color::WHITE_GRAY = Color(200, 200, 200);
 const Color Color::RED = Color(255, 0, 0);
 const Color Color::GREEN = Color(0, 255, 0);
 const Color Color::BLUE = Color(0, 0, 255);
 const Color Color::YELLOW = Color(255, 255, 0);
 const Color Color::LIGHT_YELLOW = Color(255, 255, 224);
 const Color Color::LIGHT_ORANGE = Color(255, 200, 170);