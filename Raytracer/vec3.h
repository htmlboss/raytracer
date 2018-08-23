#pragma once

#include "MathConstexpr.h"

struct vec3 {
	float x, y, z;

};

constexpr vec3 operator*(float k, const vec3& v) {
	return { k * v.x, k * v.y, k * v.z };
}

constexpr vec3 operator-(const vec3& v1, const vec3& v2) {
	return { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
}

constexpr vec3 operator+(const vec3& v1, const vec3& v2) {
	return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
}

constexpr float dot(const vec3& v1, const vec3& v2) {
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

constexpr float mag(const vec3& v) {
	return math_constexpr::sqrt(dot(v, v));
}

constexpr vec3 norm(const vec3& v) {
	return (float{ 1.0 } / mag(v)) * v;
}

constexpr vec3 cross(const vec3& v1, const vec3& v2) {
	return { v1.y * v2.z - v1.z * v2.y,
			 v1.z * v2.x - v1.x * v2.z,
			 v1.x * v2.y - v1.y * v2.x };
}