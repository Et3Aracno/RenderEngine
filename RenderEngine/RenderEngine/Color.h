#pragma once
#include <stdint.h>
#include <array>

class Color final {
private:
	std::array<uint8_t, 4> c_;

public:
	Color(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 255) : c_({ r, g, b, a }) {}
	Color(const std::array<uint8_t, 4>& color) : c_(color) {}
	Color(uint32_t color) : c_({ static_cast<uint8_t>(color & 0xFF),
								 static_cast<uint8_t>((color >> 8) & 0xFF),
								 static_cast<uint8_t>((color >> 16) & 0xFF),
								 static_cast<uint8_t>((color >> 24) & 0xFF) }) {
	}

	Color operator+(const Color& other) const;
	Color& operator=(const Color& color) = default;

	uint8_t r() const { return c_[0]; }
	uint8_t g() const { return c_[1]; }
	uint8_t b() const { return c_[2]; }
	uint8_t a() const { return c_[3]; }

	uint32_t getRgb() const;
	void setRgb(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
	static uint32_t colorScale(uint32_t c, float k);

	static const Color white;
	static const Color black;
	static const Color gray;
	static const Color whiteGray;
	static const Color red;
	static const Color green;
	static const Color blue;
	static const Color yellow;
	static const Color lightYellow;
	static const Color lightOrange;
};