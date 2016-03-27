#include <runtime_assert>
#include <Lights.hpp>

namespace haste {

//
// Light space.
//
//          n/y
//           +
//           |
//           |
//           |
//           | v0
//           /-----------+ b/x
//          /   ****
//         /        ****
//        /             ****
//       +                  ****
//      t/z                     ****
//     *                            ****
// v1 ************************************** v2
//

float AreaLights::queryTotalPower() const {
    vec3 power = queryTotalPower3();
    return power.x + power.y + power.z;
}

vec3 AreaLights::queryTotalPower3() const {
    runtime_assert(indices.size() % 3 == 0);

    vec3 result(0.0f);

    size_t numFaces = this->numFaces();

    for (size_t i = 0; i < numFaces; ++i) {
        result += queryAreaLightPower3(i);
    }

    return result;
}

float AreaLights::queryAreaLightPower(size_t id) const {
    vec3 power = queryAreaLightPower3(id);
    return power.x + power.y + power.z;
}

vec3 AreaLights::queryAreaLightPower3(size_t id) const {
    return exitances[id] * queryAreaLightArea(id);
}

float AreaLights::queryAreaLightArea(size_t id) const {
    vec3 u = vertices[indices[id * 3 + 1]] - vertices[indices[id * 3 + 0]];
    vec3 v = vertices[indices[id * 3 + 2]] - vertices[indices[id * 3 + 0]];
    return length(cross(u, v)) * 0.5f;
}

size_t AreaLights::sampleLight() const {
    auto sample = lightSampler.sample(); 
    return min(size_t(sample * numFaces()), numFaces() - 1);
}

LightPoint AreaLights::sampleSurface(size_t id) const {
    LightPoint result;

    vec3 uvw = faceSampler.sample();

    result.position =
        vertices[indices[id * 3 + 0]] * uvw.z +
        vertices[indices[id * 3 + 1]] * uvw.x +
        vertices[indices[id * 3 + 2]] * uvw.y;

    result.toWorldM =
        toWorldMs[indices[id * 3 + 0]] * uvw.z +
        toWorldMs[indices[id * 3 + 1]] * uvw.x +
        toWorldMs[indices[id * 3 + 2]] * uvw.y;

    return result;
}

LightPhoton AreaLights::emit() const {
    size_t id = sampleLight();

    LightPoint point = sampleSurface(id);

    LightPhoton result;
    result.position = point.position;
    result.direction = point.toWorldM * cosineSampler.sample();
    result.power = queryAreaLightPower3(id);

    return result;
}

LightSample AreaLights::sample(const vec3& position) const {
    // below computations are probably incorrect (to be fixed)

    size_t face = sampleLight();
    vec3 uvw = faceSampler.sample();

    vec3 normal = lerpNormal(face, uvw);
    vec3 radiance = exitances[face] * lightWeights[face];

    LightSample sample;
    sample.position = lerpPosition(face, uvw);
    sample.incident = normalize(sample.position - position);
    sample.radiance = max(vec3(0.0f), radiance * dot(normal, -sample.incident));

    return sample;
}

void AreaLights::buildLightStructs() const {
    size_t numFaces = this->numFaces();
    lightWeights.resize(numFaces);

    float totalPower = queryTotalPower();
    float totalPowerInv = 1.0f / totalPower;

    for (size_t i = 0; i < numFaces; ++i) {
        float power = queryAreaLightPower(i);
        lightWeights[i] = power * totalPowerInv;
    }

    lightSampler = PiecewiseSampler(
        lightWeights.data(),
        lightWeights.data() + lightWeights.size());

    for (size_t i = 0; i < numFaces; ++i) {
        lightWeights[i] = 1.f / lightWeights[i];
    }
}

void renderPhotons(
    vector<vec4>& image,
    size_t width,
    const vector<LightPhoton>& photons,
    const mat4& proj)
{
    size_t height = image.size() / width;
    float f5width = 0.5f * float(width);
    float f5height = 0.5f * float(height);

    for (size_t k = 0; k < photons.size(); ++k) {
        vec4 h = proj * vec4(photons[k].position, 1.0);
        vec3 v = h.xyz() / h.w;

        if (-1.0f <= v.z && v.z <= +1.0f) {
            int x = int((v.x + 1.0f) * f5width + 0.5f);
            int y = int((v.y + 1.0f) * f5height + 0.5f);

            for (int j = y - 1; j <= y + 1; ++j) {
                for (int i = x - 1; i <= x + 1; ++i) {
                    if (0 <= i && i < width && 0 <= j && j < height) {
                        image[j * width + i] = vec4(photons[k].power, 1.0f);
                        image[j * width + i] = clamp(image[j * width + i], 0.0f, 1.0f);
                    }
                }
            }
        }
    }
}

}