#pragma once

#include "vec3.h"

struct camera {
	vec3 pos;
	vec3 forward;
	vec3 right;
	vec3 up;

	constexpr camera(const vec3& pos, const vec3& look_at)
		: pos{ pos },
		forward{ norm(look_at - pos) },
		right{ 1.5f * norm(cross(forward, {0.0f, -1.0f, 0.0f})) },
		up{ 1.5f * norm(cross(forward, right)) }
	{}
};

