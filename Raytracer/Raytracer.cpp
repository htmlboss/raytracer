#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <vector>
#include <iostream>
#include <chrono>
#include <algorithm>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>

#define MAX_RAY_DEPTH 5

#if defined __linux__ || defined __APPLE__
// "Compiled for Linux
#else
// Windows doesn't define these values by default, Linux does
#define INFINITY 1e8
#endif

struct Sphere {    
    Sphere(
        const glm::vec3& c,
        const float &r,
        const glm::vec3& sc,
        const float& refl = 0.0f,
        const float& transp = 0.0f,
        const glm::vec3& ec = glm::vec3(0.0f)) :
        center(c), radius(r), radius2(r * r), surfaceColor(sc), emissionColor(ec),
        transparency(transp), reflection(refl)
    {}

    auto intersect(const glm::vec3& rayorig, const glm::vec3& raydir, float& t0, float& t1) const {
        const auto l = center - rayorig;

        const auto tca = glm::dot(l, raydir);
        if (tca < 0) return false;
        
        const auto d2 = glm::dot(l, l) - tca * tca;
        if (d2 > radius2) return false;
        
        const auto thc = glm::sqrt(radius2 - d2);
        t0 = tca - thc;
        t1 = tca + thc;

        return true;
    }

    glm::vec3 center; /// position of the sphere
    float radius, radius2; /// sphere radius and radius^2
    glm::vec3 surfaceColor, emissionColor; /// surface color and emission (light)
    float transparency, reflection; /// surface transparency and reflectivity
};

constexpr float mix(const float a, const float b, const float mix) noexcept {
    return b * mix + a * (1.0f - mix);
}

glm::vec3 trace(
    const glm::vec3& rayorig,
    const glm::vec3& raydir,
    const std::vector<Sphere> &spheres,
    const int &depth)
{
    float tnear = INFINITY;
    const Sphere* sphere = nullptr;
    // find intersection of this ray with the sphere in the scene
    for (unsigned i = 0; i < spheres.size(); ++i) {
        float t0 = INFINITY, t1 = INFINITY;
        if (spheres[i].intersect(rayorig, raydir, t0, t1)) {
            if (t0 < 0) t0 = t1;
            if (t0 < tnear) {
                tnear = t0;
                sphere = &spheres[i];
            }
        }
    }
    // if there's no intersection return black or background color
    if (!sphere) return glm::vec3(2.0f);
    
    glm::vec3 surfaceColor(0.0f); // color of the ray/surfaceof the object intersected by the ray
    const glm::vec3 phit = rayorig + raydir * tnear; // point of intersection
    glm::vec3 nhit = phit - sphere->center; // normal at the intersection point
    nhit = glm::normalize(nhit); // normalize normal direction
                      // If the normal and the view direction are not opposite to each other
                      // reverse the normal direction. That also means we are inside the sphere so set
                      // the inside bool to true. Finally reverse the sign of IdotN which we want
                      // positive
    const float bias = 1e-4; // add some bias to the point from which we will be tracing
    
    bool inside = false;
    if (glm::dot(raydir, nhit) > 0) {
        nhit = -nhit, inside = true;
    }
    
    if ((sphere->transparency > 0 || sphere->reflection > 0) && depth < MAX_RAY_DEPTH) {
        float facingratio = -glm::dot(raydir, nhit);
        // change the mix value to tweak the effect
        float fresneleffect = mix(pow(1 - facingratio, 3), 1, 0.1);
        // compute reflection direction (not need to normalize because all vectors
        // are already normalized)
        glm::vec3 refldir(raydir - nhit * 2.0f * glm::dot(raydir, nhit));
        refldir = glm::normalize(refldir);
        glm::vec3 reflection = trace(phit + nhit * bias, refldir, spheres, depth + 1);
        glm::vec3 refraction(0.0f);
        // if the sphere is also transparent compute refraction ray (transmission)
        if (sphere->transparency) {
            float ior = 1.1, eta = (inside) ? ior : 1 / ior; // are we inside or outside the surface?
            float cosi = -glm::dot(nhit, raydir);
            float k = 1 - eta * eta * (1 - cosi * cosi);
            glm::vec3 refrdir = raydir * eta + nhit * (eta * cosi - sqrt(k));
            refrdir = glm::normalize(refrdir);
            refraction = trace(phit - nhit * bias, refrdir, spheres, depth + 1);
        }
        // the result is a mix of reflection and refraction (if the sphere is transparent)
        surfaceColor = (
            reflection * fresneleffect +
            refraction * (1 - fresneleffect) * sphere->transparency) * sphere->surfaceColor;
    }
    else {
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
                            transmission = { 0.0f, 0.0f, 0.0f };
                            break;
                        }
                    }
                }
                surfaceColor += sphere->surfaceColor * transmission *
                    std::max(float(0), glm::dot(nhit, lightDirection)) * spheres[i].emissionColor;
            }
        }
    }

    return surfaceColor + sphere->emissionColor;
}

void render(const std::vector<Sphere>& spheres) {
    const constexpr unsigned width = 640, height = 480;
    
    glm::vec3 *image = new glm::vec3[width * height], *pixel = image;
    float invWidth = 1 / float(width), invHeight = 1 / float(height);
    float fov = 30, aspectratio = width / float(height);
    float angle = glm::tan(glm::pi<float>() * 0.5f * fov / 180.0f);
    
    // Trace rays
    for (unsigned y = 0; y < height; ++y) {
        for (unsigned x = 0; x < width; ++x, ++pixel) {
            
            const auto xx = (2.0f * ((x + 0.5f) * invWidth) - 1.0f) * angle * aspectratio;
            const auto yy = (1.0f - 2.0f * ((y + 0.5f) * invHeight)) * angle;
            
            glm::vec3 raydir(xx, yy, -1.0f);
            raydir = glm::normalize(raydir);
            *pixel = trace(glm::vec3(0.0f), raydir, spheres, 0);
        }
    }
    
    // Save result to a PPM image (keep these flags if you compile under Windows)
    std::ofstream ofs("./untitled.ppm", std::ios::out | std::ios::binary);
    ofs << "P6\n" << width << " " << height << "\n255\n";
    for (unsigned i = 0; i < width * height; ++i) {
        ofs << (unsigned char)(std::min(float(1), image[i].x) * 255) <<
            (unsigned char)(std::min(float(1), image[i].y) * 255) <<
            (unsigned char)(std::min(float(1), image[i].z) * 255);
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
