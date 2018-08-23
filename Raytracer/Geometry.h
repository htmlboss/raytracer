#pragma once

#include "Surface.h"


#include <optional>

class any_thing;

struct ray {
	vec3 start;
	vec3 dir;
};

struct intersection {
	const any_thing* thing_;
	ray ray_;
	float dist;
};

struct sphere {
	vec3 centre;
	float radius2;
	surface surface_;

	constexpr sphere(const vec3& centre, float radius, const surface& surface_)
		: centre{ centre },
		radius2{ radius * radius },
		surface_{ surface_ }
	{}

	constexpr
		std::optional<intersection> intersect(const any_thing* pself, const ray& ray_) const {
		
		const vec3 eo = centre - ray_.start;
		const auto v = dot(eo, ray_.dir);
		auto dist{ 0.0f };

		if (v >= 0) {
			const auto disc = radius2 - (dot(eo, eo) - v * v);
			if (disc >= 0) {
				dist = v - sqrt(disc);
			}
		}
		if (dist == 0.0f) {
			return std::nullopt;
		}
		return intersection{ pself, ray_, dist };
	}

	constexpr vec3 get_normal(const vec3& pos) const {
		return norm(pos - centre);
	}

	constexpr const surface& get_surface() const {
		return surface_;
	}
};

struct plane {
	vec3 norm;
	float offset;
	surface surface_;

	constexpr std::optional<intersection> intersect(const any_thing* pself, const ray& ray_) const {
		const auto denom = dot(norm, ray_.dir);
		if (denom > 0) {
			return std::nullopt;
		}
		const auto dist = (dot(norm, ray_.start) + offset) / (-denom);
		return intersection{ pself, ray_, dist };
	}

	constexpr vec3 get_normal(const vec3&) const {
		return norm;
	}

	constexpr const surface& get_surface() const {
		return surface_;
	}
};
