#include <runtime_assert>
#include <utility.hpp>

namespace haste {

UniformSampler::UniformSampler() {
    std::random_device device;
    engine.seed(device());
}

float UniformSampler::sample() {
    return std::uniform_real_distribution<float>()(engine);
}

PiecewiseSampler::PiecewiseSampler() { }

PiecewiseSampler::PiecewiseSampler(const float* weightsBegin, const float* weightsEnd) {
    std::random_device device;
    engine.seed(device());

    size_t numWeights = weightsEnd - weightsBegin;

    auto lambda = [&](float x) {
        return weightsBegin[min(size_t(x * numWeights), numWeights - 1)];
    };

    distribution = std::piecewise_constant_distribution<float>(
        numWeights,
        0.f,
        1.f,
        lambda);
}

float PiecewiseSampler::sample() {
    return distribution(engine);
}

vec3 BarycentricSampler::sample() {
    float u = uniform.sample();
    float v = uniform.sample();

    if (u + v <= 1) {
        return vec3(u, v, 1 - u - v);
    }
    else {
        return vec3(1 - u, 1 - v, u + v - 1);
    }
}

}

#include <ImfInputFile.h>
#include <ImfOutputFile.h>
#include <ImfChannelList.h>
#include <ImfStringAttribute.h>
#include <ImfMatrixAttribute.h>
#include <ImfArray.h>

using namespace std;
using namespace Imf;

namespace haste {

void saveEXR(
    const string& path,
    size_t widthll,
    size_t heightll,
    const vec3* data)
{
    runtime_assert(widthll < INT_MAX);
    runtime_assert(heightll < INT_MAX);

    const int width = int(widthll);
    const int height = int(heightll);

    Header header (width, height);
    header.channels().insert ("R", Channel (Imf::FLOAT));
    header.channels().insert ("G", Channel (Imf::FLOAT));
    header.channels().insert ("B", Channel (Imf::FLOAT));

    OutputFile file (path.c_str(), header);

    FrameBuffer framebuffer;

    size_t size = width * height * 3;
    vector<float> data_copy(size);

    for (size_t y = 0; y < height; ++y) {
        ::memcpy(
            data_copy.data() + y * width * 3,
            data + (height - y - 1) * width,
            width * sizeof(vec3));
    }

    auto R = Slice(Imf::FLOAT, (char*)(data_copy.data() + 0), sizeof(float) * 3, sizeof(float) * width * 3);
    auto G = Slice(Imf::FLOAT, (char*)(data_copy.data() + 1), sizeof(float) * 3, sizeof(float) * width * 3);
    auto B = Slice(Imf::FLOAT, (char*)(data_copy.data() + 2), sizeof(float) * 3, sizeof(float) * width * 3);

    framebuffer.insert("R", R);
    framebuffer.insert("G", G);
    framebuffer.insert("B", B);

    file.setFrameBuffer(framebuffer);
    file.writePixels(height);
}

void saveEXR(
    const string& path,
    size_t width,
    size_t height,
    const vec4* data)
{
    auto data3 = vector<vec3>(width * height);

    for (size_t i = 0; i < data3.size(); ++i) {
        data3[i] = data[i].xyz() / data[i].w;
    }

    saveEXR(path, width, height, data3.data());
}

void loadEXR(
    const string& path,
    size_t& width,
    size_t& height,
    vector<vec3>& data)
{
    InputFile file (path.c_str());

    auto dw = file.header().dataWindow();
    int ihegith = dw.max.x - dw.min.x + 1;
    int iwidth = dw.max.y - dw.min.y + 1;

    runtime_assert(iwidth >= 0);
    runtime_assert(ihegith >= 0);

    width = size_t(iwidth);
    height = size_t(ihegith);

    FrameBuffer framebuffer;

    data.resize(width * height);

    float* fdata = (float*)data.data();

    auto R = Slice(Imf::FLOAT, (char*)(fdata + 0), sizeof(float) * 3, sizeof(float) * width * 3);
    auto G = Slice(Imf::FLOAT, (char*)(fdata + 1), sizeof(float) * 3, sizeof(float) * width * 3);
    auto B = Slice(Imf::FLOAT, (char*)(fdata + 2), sizeof(float) * 3, sizeof(float) * width * 3);

    framebuffer.insert("R", R);
    framebuffer.insert("G", G);
    framebuffer.insert("B", B);

    file.setFrameBuffer(framebuffer);
    file.readPixels(dw.min.y, dw.max.y);

    for (size_t i = 0; i < height / 2; ++i) {
        for (size_t j = 0; j < width; ++j) {
            std::swap(data[i * width + j], data[(height - i - 1) * width + j]);
        }
    }
}

void loadEXR(
    const string& path,
    size_t& width,
    size_t& height,
    vector<vec4>& data)
{
    auto data3 = vector<vec3>();
    loadEXR(path, width, height, data3);

    data.resize(data3.size());

    for (size_t i = 0; i < data3.size(); ++i) {
        data[i] = vec4(data3[i], 1.0f);
    }
}

}

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

namespace haste {

string homePath() {
    const char* home = getenv("HOME");

    if (home == nullptr) {
        home = getpwuid(getuid())->pw_dir;
    }

    return home;
}

string baseName(string path) {
    size_t index = path.find_last_of("/");

    if (index != string::npos) {
        return path.substr(index + 1, path.size());
    }
    else {
        return path;
    }
}

string fixedPath(string base, string scene, int samples) {
    string ext;
    tie(base, ext) = splitext(base);

    if (ext.empty()) {
        ext = ".exr";
    }

    string sceneBase, sceneExt;
    tie(sceneBase, sceneExt) = splitext(scene);

    stringstream result;

    if (!base.empty() && base[base.size() - 1] != '/') {
        result << base << "." << baseName(sceneBase) << "." << samples << ext;
    }
    else {
        result << base << baseName(sceneBase) << "." << samples << ext;
    }

    return result.str();
}

pair<string, string> splitext(string path) {
    size_t index = path.find_last_of(".");

    if (index == string::npos || index == 0) {
        return make_pair(path, string());
    }
    else {
        return make_pair(path.substr(0, index), path.substr(index, path.size()));
    }
}

void renderPoints(
    vector<vec4>& image,
    size_t width,
    const vector<vec3>& points,
    const vec3& color,
    const mat4& proj)
{
    size_t height = image.size() / width;
    float f5width = 0.5f * float(width);
    float f5height = 0.5f * float(height);

    for (size_t i = 0; i < points.size(); ++i) {
        vec4 h = proj * vec4(points[i], 1.0);
        vec3 v = h.xyz() / h.w;

        if (-1.0f <= v.z && v.z <= +1.0f) {
            int x = int((v.x + 1.0f) * f5width + 0.5f);
            int y = int((v.y + 1.0f) * f5height + 0.5f);

            for (int j = y - 0; j <= y + 0; ++j) {
                for (int i = x - 0; i <= x + 0; ++i) {
                    if (0 <= i && i < width && 0 <= j && j < height) {
                        image[j * width + i] = vec4(color, 1.0f);
                    }
                }
            }

        }
    }
}

vec3 centroid(const vector<vec3>& points) {
    vec3 result = vec3(0.0f);

    for (size_t i = 0; i < points.size(); ++i) {
        result += points[i];
    }

    return result / float(points.size());
}

void renderPoints(
    vector<vec4>& image,
    size_t width,
    const vector<vec3>& points,
    const vec3& color,
    const vec3& origin)
{
    size_t height = image.size() / width;
    vec3 center = vec3(0.0f); // centroid(points);

    mat4 proj = perspective(
        pi<float>() / 3.0f,
        float(width) / float(height),
        0.1f,
        1000.0f);

    mat4 view = lookAt(origin, center, vec3(0, 1, 0));

    renderPoints(
        image,
        width,
        points,
        color,
        proj * view);
}

void renderPoints(
    vector<vec4>& image,
    size_t width,
    const vector<vec3>& points,
    const vec3& color,
    float theta,
    float phi,
    float radius)
{
    size_t height = image.size() / width;
    vec3 center = vec3(0.0f); // centroid(points);

    mat4 proj = perspective(
        pi<float>() / 3.0f,
        float(width) / float(height),
        0.1f,
        1000.0f);

    mat4 view =
        translate(vec3(0.0f, 0.0f, -radius)) *
        rotate(-theta, vec3(1, 0, 0)) *
        rotate(phi, vec3(0, 1, 0)) *
        translate(-center);

    renderPoints(
        image,
        width,
        points,
        color,
        proj * view);
}

}

#include <sys/stat.h>

namespace haste {

size_t getmtime(const string& path) {
    struct stat buf;
    stat(path.c_str(), &buf);
    return buf.st_mtime;
}

}
