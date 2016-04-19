#include <runtime_assert>
#include <Scene.hpp>
#include <Camera.hpp>
#include <streamops.hpp>
#include <cstring>

namespace haste {

Scene::Scene(
    Materials&& materials,
    vector<Mesh>&& meshes,
    AreaLights&& areaLights)
    : materials(move(materials))
    , meshes(move(meshes))
    , lights(move(areaLights))
{
    rtcScene = nullptr;

    _numIntersectRays = 0;
    _numOccludedRays = 0;
}

unsigned makeRTCMesh(RTCScene rtcScene, size_t i, const vector<Mesh>& meshes) {
    unsigned geomID = rtcNewTriangleMesh(
        rtcScene,
        RTC_GEOMETRY_STATIC,
        meshes[i].indices.size() / 3,
        meshes[i].vertices.size(),
        1);

    vec4* vbuffer = (vec4*) rtcMapBuffer(rtcScene, geomID, RTC_VERTEX_BUFFER);

    for (size_t j = 0; j < meshes[i].vertices.size(); ++j) {
        vbuffer[j].x = meshes[i].vertices[j].x;
        vbuffer[j].y = meshes[i].vertices[j].y;
        vbuffer[j].z = meshes[i].vertices[j].z;
        vbuffer[j].w = 1;
    }

    rtcUnmapBuffer(rtcScene, geomID, RTC_VERTEX_BUFFER);

    int* triangles = (int*) rtcMapBuffer(rtcScene, geomID, RTC_INDEX_BUFFER);
    std::memcpy(
        triangles,
        meshes[i].indices.data(),
        meshes[i].indices.size() * sizeof(int));

    rtcUnmapBuffer(rtcScene, geomID, RTC_INDEX_BUFFER);

    return geomID;
}

unsigned makeRTCMesh(RTCScene rtcScene, const AreaLights& lights) {
    unsigned geomID = rtcNewTriangleMesh(
        rtcScene,
        RTC_GEOMETRY_STATIC,
        lights.indices.size() / 3,
        lights.vertices.size(),
        1);

    vec4* vbuffer = (vec4*) rtcMapBuffer(rtcScene, geomID, RTC_VERTEX_BUFFER);

    for (size_t j = 0; j < lights.vertices.size(); ++j) {
        vbuffer[j].x = lights.vertices[j].x;
        vbuffer[j].y = lights.vertices[j].y;
        vbuffer[j].z = lights.vertices[j].z;
        vbuffer[j].w = 1;
    }

    rtcUnmapBuffer(rtcScene, geomID, RTC_VERTEX_BUFFER);

    int* triangles = (int*) rtcMapBuffer(rtcScene, geomID, RTC_INDEX_BUFFER);
    std::memcpy(
        triangles,
        lights.indices.data(),
        lights.indices.size() * sizeof(int));

    rtcUnmapBuffer(rtcScene, geomID, RTC_INDEX_BUFFER);

    return geomID;
}

void updateRTCScene(RTCScene& rtcScene, RTCDevice device, const Scene& scene) {
    if (rtcScene) {
        rtcDeleteScene(rtcScene);
    }

    rtcScene = rtcDeviceNewScene(
        device,
        RTC_SCENE_STATIC | RTC_SCENE_HIGH_QUALITY,
        RTC_INTERSECT1);

    if (rtcScene == nullptr) {
        throw std::runtime_error("Cannot create RTCScene.");
    }

    unsigned geomID = makeRTCMesh(rtcScene, scene.lights);
    runtime_assert(geomID == 0, "Area lights have to get 0 primID.");

    for (size_t i = 0; i < scene.meshes.size(); ++i) {
        unsigned geomID = makeRTCMesh(rtcScene, i, scene.meshes);
        runtime_assert(geomID == i + 1, "Geometry ID doesn't correspond to mesh index.");
    }

    rtcCommit(rtcScene);
}

void Scene::buildAccelStructs(RTCDevice device) {
    if (rtcScene == nullptr) {
        updateRTCScene(rtcScene, device, *this);
        lights.buildLightStructs();
    }
}

const BSDF& Scene::queryBSDF(const RayIsect& hit) const {
    runtime_assert(hit.meshId() < meshes.size());
    return *materials.bsdfs[meshes[hit.meshId()].materialID];
}

vec3 Scene::lerpNormal(const RayIsect& hit) const {
    runtime_assert(hit.meshId() < meshes.size());

    const float w = 1.f - hit.u - hit.v;
    auto& mesh = meshes[hit.meshId()];

    return w * mesh.normals[mesh.indices[hit.primID * 3 + 0]] +
         hit.u * mesh.normals[mesh.indices[hit.primID * 3 + 1]] +
         hit.v * mesh.normals[mesh.indices[hit.primID * 3 + 2]];
}

SurfacePoint Scene::querySurface(const RayIsect& isect) const {
    runtime_assert(isect.meshId() < meshes.size());

    const float w = 1.f - isect.u - isect.v;
    auto& mesh = meshes[isect.meshId()];

    SurfacePoint point;
    point.position = (vec3&)isect.org + (vec3&)isect.dir * isect.tfar;

    point.toWorldM[0] =
        normalize(w * mesh.bitangents[mesh.indices[isect.primID * 3 + 0]] +
        isect.u * mesh.bitangents[mesh.indices[isect.primID * 3 + 1]] +
        isect.v * mesh.bitangents[mesh.indices[isect.primID * 3 + 2]]);

    point.toWorldM[1] =
        normalize(w * mesh.normals[mesh.indices[isect.primID * 3 + 0]] +
        isect.u * mesh.normals[mesh.indices[isect.primID * 3 + 1]] +
        isect.v * mesh.normals[mesh.indices[isect.primID * 3 + 2]]);

    point.toWorldM[2] =
        normalize(w * mesh.tangents[mesh.indices[isect.primID * 3 + 0]] +
        isect.u * mesh.tangents[mesh.indices[isect.primID * 3 + 1]] +
        isect.v * mesh.tangents[mesh.indices[isect.primID * 3 + 2]]);

    point.materialID = mesh.materialID;

    return point;
}

vec3 Scene::queryRadiance(const RayIsect& isect) const {
    runtime_assert(isect.isLight());
    const vec3 normal = lights.lerpNormal(isect);
    const vec3 exitance = lights.exitances[isect.faceId()];

    if (dot(normal, isect.incident()) > 0.0f) {
        return exitance * one_over_pi<float>();
    }
    else {
        return vec3(0.0f);
    }
}

LightSample Scene::sampleLight(
        RandomEngine& engine,
        const vec3& position) const
{
    LightSample sample = lights.sample(engine, position);
    sample._radiance *= occluded(sample.position(), position);
    return sample;
}

RayIsect Scene::intersect(const vec3& origin, const vec3& direction) const {
    RayIsect rtcRay;
    (*(vec3*)rtcRay.org) = origin;
    (*(vec3*)rtcRay.dir) = direction;
    rtcRay.tnear = 0.00001f;
    rtcRay.tfar = INFINITY;
    rtcRay.geomID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.primID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.instID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.mask = 0xFFFFFFFF;
    rtcRay.time = 0.f;
    rtcIntersect(rtcScene, rtcRay);

    ++_numIntersectRays;

    return rtcRay;
}

RayIsect Scene::intersect(const Ray& ray) const {
    return intersect(ray.origin, ray.direction);
}

float Scene::occluded(const vec3& origin, const vec3& target) const {
    RTCRay rtcRay;
    (*(vec3*)rtcRay.org) = origin;
    (*(vec3*)rtcRay.dir) = target - origin;
    rtcRay.tnear = 0.00001f;
    rtcRay.tfar = 0.99999f;
    rtcRay.geomID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.primID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.instID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.mask = 0xFFFFFFFF;
    rtcRay.time = 0.f;
    rtcOccluded(rtcScene, rtcRay);

    ++_numOccludedRays;

    return rtcRay.geomID == 0 ? 0.f : 1.f;
}

size_t Scene::numIntersectRays() const {
    return _numIntersectRays;
}

size_t Scene::numOccludedRays() const {
    return _numOccludedRays;
}

size_t Scene::numRays() const {
    return _numIntersectRays + _numOccludedRays;
}

const DirectLightSample Scene::sampleDirectLightAngle(
    RandomEngine& engine,
    const SurfacePoint& point,
    const vec3& omegaR,
    const BSDF& bsdf) const
{







    return DirectLightSample();
}

/*
const DirectLightSample sampleDirectLightArea(
    RandomEngine& engine,
    const SurfacePoint& point,
    const vec3& omegaR,
    const BSDF& bsdf) const;

const DirectLightSample sampleDirectLightMixed(
    RandomEngine& engine,
    const SurfacePoint& point,
    const vec3& omegaR,
    const BSDF& bsdf) const;
*/
}
