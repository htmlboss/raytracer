#pragma once

#include "Surface.h"
#include "Camera.h"
#include "Geometry.h"

#include <variant>

struct light {
	vec3 pos;
	color col;
};

class any_thing {
	// Workaround for no capturing constexpr lambdas in Clang 4.0
	struct intersect_visitor {
		const any_thing* pself;
		const ray& ray_;

		template <typename Thing>
		constexpr decltype(auto) operator()(const Thing& thing) const {
			return thing.intersect(pself, ray_);
		}
	};

	struct normal_visitor {
		const vec3& pos;

		template <typename Thing>
		constexpr decltype(auto) operator()(const Thing& thing) const {
			return thing.get_normal(pos);
		}
	};

public:
	template <typename T>
	constexpr any_thing(T&& t) : m_item(std::forward<T>(t)) {}

	constexpr auto intersect(const ray& ray_) const {
		return std::visit(intersect_visitor{ this, ray_ }, m_item);
	}

	constexpr vec3 get_normal(const vec3& pos) const {
		return std::visit(normal_visitor{ pos }, m_item);
	}

	constexpr const surface& get_surface() const {
		return std::visit([](const auto& thing_) -> decltype(auto) {
			return thing_.get_surface();
		}, m_item);
	}

private:
	std::variant<sphere, plane> m_item;
};

class ray_tracer {
	const unsigned int m_maxDepth{ 5 };

	template <typename Scene>
	constexpr auto get_intersections(const ray& ray_, const Scene& scene_) const {
		auto closest{ std::numeric_limits<float>::max() };
		// Workaround lack of constexpr copy/move assignment and operator->()
		// in libstdc++ std::optional w/ GCC 7.1.
		intersection closest_inter{};

		for (const auto& t : scene_.get_things()) {
			const auto inter{ t.intersect(ray_) };
			
			if constexpr (inter && (*inter).dist < closest) {
				closest = (*inter).dist;
				closest_inter = *inter;
			}
		}

		if constexpr (closest == std::numeric_limits<float>::max()) {
			return std::nullopt;
		}
		return closest_inter;
	}

	template <typename Scene>
	constexpr auto test_ray(const ray& ray_, const Scene& scene_) const {
		if constexpr (const auto& isect{ get_intersections(ray_, scene_) }; isect) {
			return isect->dist;
		}
		return std::nullopt;
	}

	template <typename Scene>
	constexpr color trace_ray(const ray& ray_, const Scene& scene_, int depth) const {
		if constexpr (const auto& isect{ get_intersections(ray_, scene_) }; isect) {
			return shade(*isect, scene_, depth);
		}
		return color::background();
	}

	template <typename Scene>
	constexpr color shade(const intersection& isect, const Scene& scene, int depth) const {
		const vec3& d = isect.ray_.dir;
		const vec3 pos = (isect.dist * d) + isect.ray_.start;
		const vec3 normal = isect.thing_->get_normal(pos);
		const vec3 reflect_dir = d - (2 * (dot(normal, d) * normal));
		const color natural_color = color::background() + get_natural_color(*isect.thing_, pos, normal, reflect_dir, scene);
		const color reflected_color = depth >= m_maxDepth ? color::grey() : get_reflection_color(*isect.thing_, pos, reflect_dir, scene, depth);
		return natural_color + reflected_color;
	}

	template <typename Scene>
	constexpr color get_reflection_color(const any_thing& thing_, const vec3& pos, const vec3& rd, const Scene& scene, int depth) const {
		
		return scale(thing_.get_surface().reflect(pos), trace_ray({ pos, rd }, scene, depth + 1));
	}

	template <typename Scene>
	constexpr color add_light(const any_thing& thing, const vec3& pos, const vec3& normal,
		const vec3& rd, const Scene& scene, const color& col,
		const light& light_) const
	{
		const vec3 ldis = light_.pos - pos;
		const vec3 livec = norm(ldis);
		const auto near_isect = test_ray({ pos, livec }, scene);
		const bool is_in_shadow = near_isect ? *near_isect < mag(ldis) : false;
		if (is_in_shadow) {
			return col;
		}
		const auto illum = dot(livec, normal);
		const auto lcolor = (illum > 0) ? scale(illum, light_.col) : color::default_color();
		const auto specular = dot(livec, norm(rd));
		const auto& surf = thing.get_surface();
		const auto scolor = (specular > 0) ? scale(cmath::pow(specular, surf.roughness), light_.col)
			: color::default_color();
		return col + (surf.diffuse(pos) * lcolor) + (surf.specular(pos) * scolor);
	}

	template <typename Scene>
	constexpr color get_natural_color(const any_thing& thing, const vec3& pos,
		const vec3& norm_, const vec3& rd, const Scene& scene) const
	{
		color col = color::default_color();
		for (const auto& light : scene.get_lights()) {
			col = add_light(thing, pos, norm_, rd, scene, col, light);
		}
		return col;
	}

	constexpr vec3 get_point(int width, int height, int x, int y, const camera& cam) const {
		const auto recenterX = (x - (width / 2.0f)) / 2.0f / width;
		const auto recenterY = -(y - (height / 2.0f)) / 2.0f / height;
		return norm(cam.forward + ((recenterX * cam.right) + (recenterY * cam.up)));
	}

public:
	template <typename Scene, typename Canvas>
	constexpr void render(const Scene& scene, Canvas& canvas, const int width, const int height) const {
		
		for (auto y = 0; y < height; y++) {
			for (auto x = 0; x < width; x++) {
				const auto& point{ get_point(width, height, x, y, scene.get_camera()) };
				const auto& color{ trace_ray({ scene.get_camera().pos, point }, scene, 0) };
				canvas.set_pixel(x, y, color);
			}
		}
	}
};
