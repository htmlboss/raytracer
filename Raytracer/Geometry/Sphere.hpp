#pragma once

#include <glm/geometric.hpp>

struct Sphere {
	Sphere(
		const glm::vec3& c,
		const float& r,
		const glm::vec3& sc,
		const float& refl = 0.0f,
		const float& transp = 0.0f,
		const glm::vec3& ec = glm::vec3(0.0f)) :
		center(c)
		, radius(r)
		, radius2(r * r)
		, surfaceColor(sc)
		, emissionColor(ec)
		, transparency(transp)
		, reflection(refl) {}

	// Geometrically solve ray-sphere intersection
	auto intersect(const glm::vec3& rayorig, const glm::vec3& raydir, float& t0, float& t1) const {
		const auto l = center - rayorig;

		const auto tca = glm::dot(l, raydir);
		if (tca < 0)
			return false;

		const auto d2 = glm::dot(l, l) - tca * tca;
		if (d2 > radius2)
			return false;

		const auto thc = glm::sqrt(radius2 - d2);
		t0 = tca - thc;
		t1 = tca + thc;

		return true;
	}

	glm::vec3 center; // position of the sphere
	float radius, radius2; // sphere radius and radius^2
	glm::vec3 surfaceColor, emissionColor; // surface color and emission (light)
	float transparency, reflection; // surface transparency and reflectivity
};

