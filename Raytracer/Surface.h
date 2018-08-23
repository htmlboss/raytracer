#pragma once

#include "vec3.h"
#include "Color.h"

struct surface {
	using diffuse_func_t = color(*)(const vec3&);
	using specular_func_t = color(*)(const vec3&);
	using reflect_func_t = float(*)(const vec3&);

	diffuse_func_t diffuse = nullptr;
	specular_func_t specular = nullptr;
	reflect_func_t reflect = nullptr;
	int roughness{ 0 };
};

namespace surfaces {

	inline constexpr surface shiny{
			[](const vec3&) { return color::white(); },
			[](const vec3&) { return color::grey(); },
			[](const vec3&) { return float{0.7f}; },
			250
	};

	inline constexpr surface checkerboard{
			[](const vec3& pos) {
				
				if (int(math_constexpr::floor(pos.z) + math_constexpr::floor(pos.x)) % 2 != 0) {
					return color::white();
				}
				return color::black();
			},

			[](const vec3&) { return color::white(); },

			[](const vec3& pos) -> float {
				if (int(math_constexpr::floor(pos.z) + math_constexpr::floor(pos.x)) % 2 != 0) {
					return 0.1f;
				}
				return 0.7f;

			},
			150
	};

} // end namespace surfaces