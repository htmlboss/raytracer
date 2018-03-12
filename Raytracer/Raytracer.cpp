
// http://www.kevinbeason.com/smallpt/

#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h> // Make : g++ -O3 -fopenmp smallpt.cpp -o smallpt 

#include <random>
#include <chrono>
#include <iostream>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

std::default_random_engine generator;
std::uniform_real_distribution<double> distr(0.0, 1.0);
template<typename T>
constexpr auto erand48(T) {
    return distr(generator);
}

struct Vec {   
    constexpr Vec(const double x_ = 0, const double y_ = 0, const double z_ = 0) noexcept : x(x_), y(y_), z(z_) {}
    
    constexpr auto operator+(const Vec& b) const noexcept {
        return Vec(x + b.x, y + b.y, z + b.z);
    }
    
    constexpr auto operator-(const Vec& b) const noexcept {
        return Vec(x - b.x, y - b.y, z - b.z);
    }

    constexpr auto operator*(double b) const noexcept {
        return Vec(x*b, y*b, z*b);
    }
    
    constexpr auto mult(const Vec& b) const noexcept {
        return Vec(x*b.x, y*b.y, z*b.z);
    }

    constexpr auto operator%(const Vec& b) const noexcept {
        return Vec(y*b.z - z * b.y, z*b.x - x * b.z, x*b.y - y * b.x);
    }
    
    Vec& norm() {
        return *this = *this * (1 / glm::sqrt(x*x + y * y + z * z));
    }

    constexpr auto dot(const Vec& b) const noexcept {
        return x * b.x + y * b.y + z * b.z;
    }

    // position, also color (r,g,b) 
    double x, y, z;
};

struct Ray {
    constexpr Ray(const Vec& o_, const Vec& d_) : o(o_), d(d_) {}

    Vec o, d;
};

// material types, used in radiance() 
enum Refl_t {
    DIFF, SPEC, REFR
};

struct Sphere {
    constexpr Sphere(const double rad_, const Vec& p_, const Vec& e_, const Vec& c_, const Refl_t refl_) noexcept : 
        rad(rad_), p(p_), e(e_), c(c_), refl(refl_) {}
    
    double intersect(const Ray& r) const { // returns distance, 0 if nohit 
        const Vec op = p - r.o; // Solve t^2*d.d + 2*t*(o-p).d + (o-p).(o-p)-R^2 = 0 
        
        double t, eps = 1e-4, b = op.dot(r.d), det = b * b - op.dot(op) + rad * rad;
        
        if (det < 0) {
            return 0;
        }
        
        det = glm::sqrt(det);
        return (t = b - det) > eps ? t : (t = b + det) > eps ? t : 0;
    }

    double rad;       // radius 
    Vec p, e, c;      // position, emission, color 
    Refl_t refl;      // reflection type (DIFFuse, SPECular, REFRactive) 
};

const constexpr Sphere spheres[] {//Scene: radius, position, emission, color, material 
    Sphere(1e5, Vec(1e5 + 1,40.8,81.6), Vec(),Vec(.75,.25,.25),DIFF),//Left 
    Sphere(1e5, Vec(-1e5 + 99,40.8,81.6),Vec(),Vec(.25,.25,.75),DIFF),//Rght 
    Sphere(1e5, Vec(50,40.8, 1e5),     Vec(),Vec(.75,.75,.75),DIFF),//Back 
    Sphere(1e5, Vec(50,40.8,-1e5 + 170), Vec(),Vec(),           DIFF),//Frnt 
    Sphere(1e5, Vec(50, 1e5, 81.6),    Vec(),Vec(.75,.75,.75),DIFF),//Botm 
    Sphere(1e5, Vec(50,-1e5 + 81.6,81.6),Vec(),Vec(.75,.75,.75),DIFF),//Top 
    Sphere(16.5,Vec(27,16.5,47),       Vec(),Vec(1,1,1)*.999, SPEC),//Mirr 
    Sphere(16.5,Vec(73,16.5,78),       Vec(),Vec(1,1,1)*.999, REFR),//Glas 
    Sphere(600, Vec(50,681.6 - .27,81.6),Vec(12,12,12),  Vec(), DIFF) //Lite 
};

inline auto toInt(const double x) {
    return static_cast<int>(glm::pow(glm::clamp(x, 0.0, 1.0), 1 / 2.2) * 255 + 0.5);
}

inline bool intersect(const Ray& r, double& t, int& id) {
    double n = sizeof(spheres) / sizeof(Sphere), d;
    const constexpr auto infinity = 1e20;
    t = infinity;
    
    for (auto i = int(n); --i;) {
        if ((d = spheres[i].intersect(r)) && d < t) {
            t = d;
            id = i;
        }
    }
    
    return t < infinity;
}

Vec radiance(const Ray& r, int depth, unsigned short* Xi) {
    double t;                               // distance to intersection 
    int id = 0;                               // id of intersected object 
    
    if (!intersect(r, t, id)) {
        return Vec(); // if miss, return black
    }
    
    const Sphere &obj = spheres[id];        // the hit object 
    
    Vec x = r.o + r.d*t, n = (x - obj.p).norm(), nl = n.dot(r.d)<0 ? n : n * -1, f = obj.c;
    
    const double p = f.x>f.y && f.x>f.z ? f.x : f.y>f.z ? f.y : f.z; // max refl 
    
    if (++depth > 5) if (erand48(Xi)<p) f = f * (1 / p); else return obj.e; //R.R. 
    
    if (obj.refl == DIFF) {                  // Ideal DIFFUSE reflection 
        double r1 = 2 * glm::pi<double>() * erand48(Xi), r2 = erand48(Xi), r2s = glm::sqrt(r2);
        
        Vec w = nl, u = ((fabs(w.x)>.1 ? Vec(0, 1) : Vec(1)) % w).norm(), v = w % u;
        Vec d = (u*cos(r1)*r2s + v * sin(r1)*r2s + w * glm::sqrt(1 - r2)).norm();
        
        return obj.e + f.mult(radiance(Ray(x, d), depth, Xi));
    }
    if (obj.refl == SPEC)            // Ideal SPECULAR reflection 
        return obj.e + f.mult(radiance(Ray(x, r.d - n * 2 * n.dot(r.d)), depth, Xi));
    
    Ray reflRay(x, r.d - n * 2 * n.dot(r.d));     // Ideal dielectric REFRACTION 
    const bool into = n.dot(nl) > 0;                // Ray from outside going in? 
    
    double nc = 1, nt = 1.5, nnt = into ? nc / nt : nt / nc, ddn = r.d.dot(nl), cos2t;
    
    if ((cos2t = 1 - nnt * nnt*(1 - ddn * ddn)) < 0) {    // Total internal reflection 
        return obj.e + f.mult(radiance(reflRay, depth, Xi));
    }
    
    Vec tdir = (r.d*nnt - n * ((into ? 1 : -1)*(ddn*nnt + glm::sqrt(cos2t)))).norm();
    double a = nt - nc, b = nt + nc, R0 = a * a / (b*b), c = 1 - (into ? -ddn : tdir.dot(n));
    double Re = R0 + (1 - R0)*c*c*c*c*c, Tr = 1 - Re, P = .25 + .5*Re, RP = Re / P, TP = Tr / (1 - P);
    
    return obj.e + f.mult(depth>2 ? (erand48(Xi)<P ?   // Russian roulette 
        radiance(reflRay, depth, Xi)*RP : radiance(Ray(x, tdir), depth, Xi)*TP) :
        radiance(reflRay, depth, Xi)*Re + radiance(Ray(x, tdir), depth, Xi)*Tr);
}

int main(const int argc, const char* argv[]) {

    const constexpr std::size_t width = 1024, height = 768;
    const std::size_t samples = argc == 2 ? atoi(argv[1]) / 4 : 2; // # samples 
    
    Ray cam(Vec(50, 52, 295.6), Vec(0, -0.042612, -1).norm()); // cam pos, dir 
    
    Vec cx = Vec(width*.5135 / height), cy = (cx%cam.d).norm()*.5135, r, *c = new Vec[width*height];

    using namespace std::chrono;
    const auto start = duration_cast<milliseconds>(system_clock::now().time_since_epoch());

#pragma omp parallel for schedule(dynamic, 1) private(r)       // OpenMP 
    for (auto y = 0; y < height; ++y) {                       // Loop over image rows 
        
        std::fprintf(stderr, "\rRendering (%llu spp) %5.2f%%", samples * 4, 100.*y / (height - 1));
        
        for (unsigned short x = 0, Xi[3] = { 0, 0, y*y*y }; x < width; ++x)   // Loop cols 
            
            for (int sy = 0, i = (height - y - 1)*width + x; sy<2; ++sy)     // 2x2 subpixel rows 
                for (int sx = 0; sx < 2; ++sx, r = Vec()) {        // 2x2 subpixel cols 
                    for (int s = 0; s < samples; ++s) {
                        const double r1 = 2 * erand48(Xi), dx = r1<1 ? glm::sqrt(r1) - 1 : 1 - glm::sqrt(2 - r1);
                        const double r2 = 2 * erand48(Xi), dy = r2<1 ? glm::sqrt(r2) - 1 : 1 - glm::sqrt(2 - r2);
                        Vec d = cx * (((sx + .5 + dx) / 2 + x) / width - .5) +
                            cy * (((sy + .5 + dy) / 2 + y) / height - .5) + cam.d;
                        r = r + radiance(Ray(cam.o + d * 140, d.norm()), 0, Xi)*(1. / samples);
                    } // Camera rays are pushed ^^^^^ forward to start in interior 
                    
                    c[i] = c[i] + Vec(glm::clamp(r.x, 0.0, 1.0), glm::clamp(r.y, 0.0, 1.0), glm::clamp(r.z, 0.0, 1.0)) * 0.25;
                }
    }

    const auto end = duration_cast<milliseconds>(system_clock::now().time_since_epoch());

    std::cout << "\nTOTAL RUNNING TIME (ms): " << (end - start).count() << '\n';
    
    // Write image to PPM file. 
    FILE *f = fopen("image.ppm", "w");
    std::fprintf(f, "P3\n%llu %llu\n%d\n", width, height, 255);
    for (int i = 0; i < width * height; ++i) {
        fprintf(f, "%d %d %d ", toInt(c[i].x), toInt(c[i].y), toInt(c[i].z));
    }


    delete[] c;
    int x;
    std::cin >> x;
}
