#pragma once
#include <Technique.hpp>
#include <iostream>

namespace haste {

class MBPT : public Technique {
public:
    MBPT(size_t minSubpath = 3, float roulette = 0.5f, float beta = 0.0f);

    void render(
        ImageView& view,
        RandomEngine& engine,
        size_t cameraId) override;

    string name() const override;
private:
    struct LightVertex {
        SurfacePoint surface;
        vec3 omega;
        vec3 throughput;
        float a, A;

        const vec3& position() const { return surface.position(); }
        const vec3& normal() const { return surface.normal(); }
        const vec3& gnormal() const { return surface.gnormal(); }
    };

    struct EyeVertex {
        SurfacePoint surface;
        vec3 omega;
        vec3 throughput;
        float specular;
        float c, C;

        const vec3& position() const { return surface.position(); }
        const vec3& normal() const { return surface.normal(); }
        const vec3& gnormal() const { return surface.gnormal(); }
    };

    static const size_t _maxSubpath = 1024;
    const size_t _minSubpath;
    const float _roulette;
    const float _beta;

    float _pow(float x) const {
        return pow(x, _beta);
    }

    void _trace(RandomEngine& engine, size_t& size, LightVertex* path);
    vec3 _trace(RandomEngine& engine, const Ray& ray);
    vec3 _connect0(RandomEngine& engine, const EyeVertex& eye);
    vec3 _connect1(RandomEngine& engine, const EyeVertex& eye);
    vec3 _connect(const EyeVertex& eye, const LightVertex& light);
    vec3 _connect(RandomEngine& engine, const EyeVertex& eye, size_t size, const LightVertex* path);
};

}
