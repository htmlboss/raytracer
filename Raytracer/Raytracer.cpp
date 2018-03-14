
#include "Geometry/Sphere.hpp"

#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <vector>
#include <iostream>
#include <chrono>
#include <algorithm>

#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>

#define MAX_RAY_DEPTH 5

#if defined __linux__ || defined __APPLE__
// "Compiled for Linux
#else
// Windows doesn't define these values by default, Linux does
#define INFINITY 1e8
#endif

// https://smcgro.wordpress.com/2016/02/21/compile-time-raytracer-and-c-metaprogramming/

constexpr float mix(const float a, const float b, const float mix) noexcept {
	return b * mix + a * (1.0f - mix);
}

glm::vec3 trace(
	const glm::vec3& rayorig,
	const glm::vec3& raydir,
	const std::vector<Sphere>& spheres,
	const int& depth) {
	float tnear = INFINITY;
	const Sphere* sphere = nullptr;
	// find intersection of this ray with the sphere in the scene
	for (unsigned i = 0; i < spheres.size(); ++i) {
		float t0 = INFINITY, t1 = INFINITY;
		if (spheres[i].intersect(rayorig, raydir, t0, t1)) {
			if (t0 < 0)
				t0 = t1;
			if (t0 < tnear) {
				tnear = t0;
				sphere = &spheres[i];
			}
		}
	}
	// if there's no intersection return black or background color
	if (!sphere)
		return glm::vec3(2.0f);

	glm::vec3 surfaceColor(0.0f); // color of the ray/surfaceof the object intersected by the ray
	const auto phit = rayorig + raydir * tnear; // point of intersection
	glm::vec3 nhit = phit - sphere->center; // normal at the intersection point
	nhit = glm::normalize(nhit); // normalize normal direction
	// If the normal and the view direction are not opposite to each other
	// reverse the normal direction. That also means we are inside the sphere so set
	// the inside bool to true. Finally reverse the sign of Iglm::dotN which we want
	// positive
	const float bias = 1e-4; // add some bias to the point from which we will be tracing

	bool inside = false;
	if (glm::dot(raydir, nhit) > 0) {
		nhit = -nhit;
		inside = true;
	}

	if ((sphere->transparency > 0 || sphere->reflection > 0) && depth < MAX_RAY_DEPTH) {
		const auto facingratio = -glm::dot(raydir, nhit);
		
		// change the mix value to tweak the effect
		const auto fresneleffect = mix(glm::pow(1.0f - facingratio, 3.0f), 1.0f, 0.1f);
		
		// compute reflection direction (no need to normalize because all vectors
		// are already normalized)
		glm::vec3 refldir(raydir - nhit * 2.0f * glm::dot(raydir, nhit));
		refldir = glm::normalize(refldir);
		// Calculate reflection contribution
		const auto reflection = trace(phit + nhit * bias, refldir, spheres, depth + 1);
		
		// if the sphere is also transparent compute refraction ray (transmission)
		glm::vec3 refraction(0.0f);
		if (sphere->transparency) {
			const float ior = 1.1f;
			const float eta = inside ? ior : 1.0f / ior; // are we inside or outside the surface?
			const float cosi = -glm::dot(nhit, raydir);
			const float k = 1.0f - eta * eta * (1.0f - cosi * cosi);
			
			glm::vec3 refrdir = raydir * eta + nhit * (eta * cosi - glm::sqrt(k));
			refrdir = glm::normalize(refrdir);
			refraction = trace(phit - nhit * bias, refrdir, spheres, depth + 1);
		}

		// the result is a mix of reflection and refraction (if the sphere is transparent)
		surfaceColor = (   reflection * fresneleffect +
			               refraction * (1.0f - fresneleffect) * sphere->transparency) * sphere->surfaceColor;
	} else {
		// it's a diffuse object, no need to raytrace any further
		for (unsigned i = 0; i < spheres.size(); ++i) {
			if (spheres[i].emissionColor.x > 0) {
				// this is a light
				glm::vec3 transmission(1.0f);
				glm::vec3 lightDirection = spheres[i].center - phit;
				lightDirection = glm::normalize(lightDirection);

				for (unsigned j = 0; j < spheres.size(); ++j) {
					if (i != j) {
						float t0, t1;
						if (spheres[j].intersect(phit + nhit * bias, lightDirection, t0, t1)) {
							transmission = {0.0f, 0.0f, 0.0f};
							break;
						}
					}
				}
				surfaceColor += sphere->surfaceColor * transmission *
						std::max(0.0f, glm::dot(nhit, lightDirection)) * spheres[i].emissionColor;
			}
		}
	}

	return surfaceColor + sphere->emissionColor;
}

void render(const std::vector<Sphere>& spheres) {

	// Image settings
	const constexpr unsigned width = 640, height = 480;
	const constexpr float invWidth = 1.0f / static_cast<float>(width);
	const constexpr float invHeight = 1.0f / static_cast<float>(height);
	// Camera settings
	const constexpr float fov = 30.0f;
	const constexpr float aspectratio = width / static_cast<float>(height);
	const float angle = glm::tan(glm::pi<float>() * 0.5f * fov / 180.0f);

	// Allocate memory for image
	glm::vec3* image = new glm::vec3[width * height];
	glm::vec3* pixel = image; // Pointer to some pixel in the image.

	// Trace rays
	for (unsigned y = 0; y < height; ++y) {
		for (unsigned x = 0; x < width; ++x, ++pixel) {

			const float xx = (2.0f * ((x + 0.5f) * invWidth) - 1.0f) * angle * aspectratio;
			const float yy = (1.0f - 2.0f * ((y + 0.5f) * invHeight)) * angle;

			glm::vec3 raydir(xx, yy, -1.0f);
			raydir = glm::normalize(raydir);
			*pixel = trace(glm::vec3(0.0f), raydir, spheres, 0);
		}
	}

	// Save result to a PPM image (keep these flags if you compile under Windows)
	std::ofstream ofs("./untitled.ppm", std::ios::out | std::ios::binary);
	ofs << "P6\n" << width << " " << height << "\n255\n"; // PPM header
	for (unsigned int i = 0; i < width * height; ++i) {
		ofs <<	static_cast<unsigned char>(std::min(1.0f, image[i].x) * 255) <<
				static_cast<unsigned char>(std::min(1.0f, image[i].y) * 255) <<
				static_cast<unsigned char>(std::min(1.0f, image[i].z) * 255);
	}

	// Cleanup
	ofs.close();
	delete[] image;
}

int main(const int argc, const char* argv[]) {

	srand(13);
	std::vector<Sphere> spheres;
	// position, radius, surface color, reflectivity, transparency, emission color
	spheres.push_back(Sphere(glm::vec3(0.0, -10004, -20), 10000, glm::vec3(0.20, 0.20, 0.20), 0, 0.0));
	spheres.push_back(Sphere(glm::vec3(0.0, 0, -20), 4, glm::vec3(1.00, 0.32, 0.36), 1, 0.5));
	spheres.push_back(Sphere(glm::vec3(5.0, -1, -15), 2, glm::vec3(0.90, 0.76, 0.46), 1, 0.0));
	spheres.push_back(Sphere(glm::vec3(5.0, 0, -25), 3, glm::vec3(0.65, 0.77, 0.97), 1, 0.0));
	spheres.push_back(Sphere(glm::vec3(-5.5, 0, -15), 3, glm::vec3(0.90, 0.90, 0.90), 1, 0.0));
	// light
	spheres.push_back(Sphere(glm::vec3(0.0, 20, -30), 3, glm::vec3(0.00, 0.00, 0.00), 0, 0.0, glm::vec3(3)));

	using namespace std::chrono;
	const auto start = duration_cast<milliseconds>(system_clock::now().time_since_epoch());

	render(spheres);

	const auto end = duration_cast<milliseconds>(system_clock::now().time_since_epoch());

	std::cout << "TOTAL RUNNING TIME (ms): " << (end - start).count() << '\n';

	int x;
	std::cin >> x;

}
