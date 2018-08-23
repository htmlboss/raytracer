#pragma once

struct color {
	float r, g, b;

	static constexpr color white() noexcept { return { 1.0f, 1.0f, 1.0f }; }
	static constexpr color grey() noexcept { return { 0.5f, 0.5f, 0.5f }; }
	static constexpr color black() noexcept { return {}; };
	static constexpr color background() noexcept { return black(); }
	static constexpr color default_color() noexcept { return black(); }

	constexpr color scale(const float k) const noexcept {
		return { k * r, k * g, k * b };
	}

	constexpr color operator+(const color& v2) const noexcept {
		return { r + v2.r, g + v2.g, b + v2.b };
	}

	constexpr color operator*(const color& v2) const noexcept {
		return { r * v2.r, g * v2.g, b * v2.b };
	}
};

constexpr color scale(float k, const color& v) {
	return { k * v.r, k * v.g, k * v.b };
}

constexpr color operator+(const color& v1, const color& v2) {
	return { v1.r + v2.r, v1.g + v2.g, v1.b + v2.b };
}

constexpr color operator*(const color& v1, const color& v2) {
	return { v1.r * v2.r, v1.g * v2.g, v1.b * v2.b };
}