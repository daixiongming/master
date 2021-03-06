#include <streamops.hpp>
#include <runtime_assert>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <utility.hpp>
#include <loader.hpp>

namespace haste {

using std::move;

template <class Stream> Stream& operator<<(Stream& stream, const aiString& string) {
    return stream << string.C_Str();
}

template <class Stream> Stream& operator<<(Stream& stream, const aiColor3D& color) {
    return stream << "aiColor3D(" << color.r << ", " << color.g << ", " << color.b << ")";
}

template <class Stream> Stream& operator<<(Stream& stream, const aiVector2D& vector) {
    return stream << "aiVector2D(" << vector.x << ", " << vector.y << ")";
}

template <class Stream> Stream& operator<<(Stream& stream, const aiMatrix4x4& matrix) {
    return stream
        << "aiMatrix4x4("
        << "[" << matrix.a1 << ", " << matrix.a2 << ", " << matrix.a3 << ", " << matrix.a4 << "], "
        << "[" << matrix.b1 << ", " << matrix.b2 << ", " << matrix.b3 << ", " << matrix.b4 << "], "
        << "[" << matrix.c1 << ", " << matrix.c2 << ", " << matrix.c3 << ", " << matrix.c4 << "], "
        << "[" << matrix.d1 << ", " << matrix.d2 << ", " << matrix.d3 << ", " << matrix.d4 << "])";
}

template <class Stream> Stream& operator<<(Stream& stream, const aiVector3D& vector) {
    return stream << "aiVector3D(" << vector.x << ", " << vector.y << ", " << vector.z << ")";
}

template <class Stream> Stream& operator<<(Stream& stream, aiLightSourceType type) {
    switch (type) {
        case aiLightSource_UNDEFINED: return stream << "aiLightSource_UNDEFINED";
        case aiLightSource_DIRECTIONAL: return stream << "aiLightSource_DIRECTIONAL";
        case aiLightSource_POINT: return stream << "aiLightSource_POINT";
        case aiLightSource_SPOT: return stream << "aiLightSource_SPOT";
        case aiLightSource_AREA: return stream << "aiLightSource_AREA";
        default: return stream << "undefined";
    }
}

template <class Stream> Stream& operator<<(Stream& stream, const aiCamera& camera) {
    return stream
        << "aiCamera { "
        << "mAspect = " << camera.mAspect << ", "
        << "mClipPlaneFar = " << camera.mClipPlaneFar << ", "
        << "mClipPlaneNear = " << camera.mClipPlaneNear << ", "
        << "mHorizontalFOV = " << camera.mHorizontalFOV << ", "
        << "mLookAt = " << camera.mLookAt << ", "
        << "mName = " << camera.mName << ", "
        << "mPosition = " << camera.mPosition << ", "
        << "mUp = " << camera.mUp << " }";
}

template <class Stream> Stream& operator<<(Stream& stream, const aiLight& light) {
    return stream
        << "aiLight { "
        << "mAngleInnerCone = " << light.mAngleInnerCone << ", "
        << "mAngleOuterCone = " << light.mAngleOuterCone << ", "
        << "mAttenuationConstant = " << light.mAttenuationConstant << ", "
        << "mAttenuationLinear = " << light.mAttenuationLinear << ", "
        << "mAttenuationQuadratic = " << light.mAttenuationQuadratic << ", "
        << "mColorAmbient = " << light.mColorAmbient << ", "
        << "mColorDiffuse = " << light.mColorDiffuse << ", "
        << "mColorSpecular = " << light.mColorSpecular << ", "
        << "mDirection = " << light.mDirection << ", "
        << "mName = " << light.mName << ", "
        << "mPosition = " << light.mPosition << ", "
        << "mType = " << light.mType << " }";
}

template <class Stream> Stream& operator<<(Stream& stream, const aiNode& node) {
    return stream
        << "aiNode { mChildren, mMeshes, "
        << "mName = " << node.mName << ", "
        << "mNumChildren = " << node.mNumChildren << ", "
        << "mNumMeshes = " << node.mNumMeshes << ", "
        << "mParent"
        << (node.mParent ? (const char*)"" : (const char*)" = nullptr") << ", "
        << "mTransformation }";
}

std::string dirname(const std::string& path) {
    auto index = path.find_last_of("/\\");

    if (index != std::string::npos) {
        while (index != 0 && (path[index - 1] == '\\' || path[index - 1] == '/')) {
            --index;
        }

        return path.substr(0, index);
    }
    else {
        return path;
    }
}

string toString(const aiString& s) {
    return s.C_Str();
}

vec2 toVec2(const aiVector2D& v) {
    return vec2(v.x, v.y);
}

vec3 toVec3(const aiVector3D& v) {
    return vec3(v.x, v.y, v.z);
}

vec3 toVec3(const aiColor3D& v) {
    return vec3(v.r, v.g, v.b);
}

string name(const aiMaterial* material) {
    aiString name;
    material->Get(AI_MATKEY_NAME, name);
    return name.C_Str();
}

vec3 ambient(const aiMaterial* material) {
    aiColor3D result;
    material->Get(AI_MATKEY_COLOR_AMBIENT, result);
    return vec3(result.r, result.g, result.b);
}

vec3 diffuse(const aiMaterial* material) {
    aiColor3D result;
    material->Get(AI_MATKEY_COLOR_DIFFUSE, result);
    return vec3(result.r, result.g, result.b);
}

vec3 emissive(const aiMaterial* material) {
    aiColor3D result;
    material->Get(AI_MATKEY_COLOR_EMISSIVE, result);
    return vec3(result.r, result.g, result.b);
}

vec3 specular(const aiMaterial* material) {
    aiColor3D result;
    material->Get(AI_MATKEY_COLOR_SPECULAR, result);
    return vec3(result.r, result.g, result.b);
}

vec3 transparent(const aiMaterial* material) {
    aiColor3D result;
    material->Get(AI_MATKEY_COLOR_TRANSPARENT, result);
    return toVec3(result);
}

template <class T> T property(const aiMaterial* material, const char* key) {
    return T();
}

template <> bool property(const aiMaterial* material, const char* key) {
    int result = 0;
    material->Get(key, 0, 0, result);
    return result != 0;
}

template <> float property(const aiMaterial* material, const char* key) {
    float result = 0;
    material->Get(key, 0, 0, result);
    return result;
}

bool wireframe(const aiMaterial* material) {
    bool result = false;
    material->Get(AI_MATKEY_ENABLE_WIREFRAME, result);
    return result;
}

bool twosided(const aiMaterial* material) {
    bool result = false;
    material->Get(AI_MATKEY_TWOSIDED, result);
    return result;
}

int shadingModel(const aiMaterial* material) {
    int result = 0;
    material->Get(AI_MATKEY_SHADING_MODEL, result);
    return result;
}

int blendFunc(const aiMaterial* material) {
    int result = 0;
    material->Get(AI_MATKEY_BLEND_FUNC, result);
    return result;
}

float opacity(const aiMaterial* material) {
    float result = 1.0f;
    material->Get(AI_MATKEY_OPACITY, result);
    return result;
}

float shininess(const aiMaterial* material) {
    float result = 0.0f;
    material->Get(AI_MATKEY_SHININESS, result);
    return result;
}

float shininessStrength(const aiMaterial* material) {
    float result = 1.0f;
    material->Get(AI_MATKEY_SHININESS_STRENGTH, result);
    return result;
}

float refracti(const aiMaterial* material) {
    float result = 1.0f;
    material->Get(AI_MATKEY_REFRACTI, result);
    return result;
}

vec3 reflectivity(const aiMaterial* material) {
    aiColor3D result;
    material->Get(AI_MATKEY_COLOR_REFLECTIVE, result);
    return toVec3(result);
}

float reflective(const aiMaterial* material) {
    float result = 0.0f;
    material->Get(AI_MATKEY_REFLECTIVITY, result);
    return result;
}

template <class Stream> Stream& operator<<(
    Stream& stream,
    const aiMaterial& material)
{
    stream << "aiMaterial { ";

    for (unsigned i = 0; i < material.mNumProperties; ++i) {
        if (material.mProperties[i]->mType == aiPTI_String) {
            aiString result;
            aiGetMaterialString(
                &material,
                material.mProperties[i]->mKey.C_Str(),
                0, 0,
                &result);

            stream << material.mProperties[i]->mKey << " = \"" << result << "\", ";
        }
        else {
            float result[8];
            unsigned pMax = 8;

            aiGetMaterialFloatArray(
                &material,
                material.mProperties[i]->mKey.C_Str(),
                0, 0,
                result,
                &pMax);

            unsigned size = material.mProperties[i]->mDataLength / sizeof(float);

            stream << material.mProperties[i]->mKey << " = [";

            if (size != 0) {
                stream << result[0];

                for (unsigned i = 1; i < size; ++i) {
                    stream  << ", " << result[i];
                }
            }

            stream << "], ";
        }
    }

    return stream;

//    return stream
//        << "aiMaterial { "
//        << "name = " << name(&material) << ", "
//        << "COLOR_DIFFUSE = " << diffuse(&material) << ", "
//        << "COLOR_SPECULAR = " << specular(&material) << ", "
//        << "COLOR_AMBIENT = " << ambient(&material) << ", "
//        << "COLOR_EMISSIVE = " << emissive(&material) << ", "
//        << "COLOR_TRANSPARENT = " << transparent(&material) << ", "
//        // << "WIREFRAME = " << wireframe(&material) << ", "
//        << "TWOSIDED = " << twosided(&material) << ", "
//        << "SHADING_MODEL = " << shadingModel(&material) << ", "
//        << "BLEND_FUNC = " << blendFunc(&material) << ", "
//        << "OPACITY = " << opacity(&material) << ", "
//        << "SHININESS = " << shininess(&material) << ", "
//        << "SHININESS_STRENGTH = " << shininessStrength(&material) << ", "
//        << "REFRACTI = " << refracti(&material) << " }";
// #define AI_MATKEY_REFLECTIVITY "$mat.reflectivity",0,0
// #define AI_MATKEY_COLOR_REFLECTIVE "$clr.reflective",0,0
        // TEXTURE(t,n)
        // TEXBLEND(t,n)
        // TEXOP(t,n)
        // MAPPING(t,n)
        // UVWSRC(t,n)
        // MAPPINGMODE_U(t,n)
        // MAPPINGMODE_V(t,n)
        // TEXMAP_AXIS(t,n)
        // TEXFLAGS(t,n)
}




bool isEmissive(const aiScene* scene, size_t meshID) {
    size_t materialID = scene->mMeshes[meshID]->mMaterialIndex;
    auto material = scene->mMaterials[materialID];
    return emissive(material) != vec3(0.0f);
}

AreaLights loadAreaLights(const aiScene* scene) {
    AreaLights result;

    for (size_t i = 0; i < scene->mNumLights; ++i) {
        if (scene->mLights[i]->mType == aiLightSource_AREA) {
            auto light = scene->mLights[i];

            result.addLight(
                toString(light->mName),
                toVec3(light->mPosition),
                normalize(toVec3(light->mDirection)),
                normalize(toVec3(light->mUp)),
                toVec3(light->mColorDiffuse),
                toVec2(light->mSize));
        }
    }

    return result;
}

Cameras loadCameras(const aiScene* scene) {
    Cameras cameras;

    for (size_t i = 0; i < scene->mNumCameras; ++i) {
        auto camera = scene->mCameras[i];
        cameras.addCameraFovX(
            toString(camera->mName),
            toVec3(camera->mPosition),
            normalize(toVec3(camera->mLookAt)),
            normalize(toVec3(camera->mUp)),
            camera->mHorizontalFOV,
            camera->mClipPlaneNear,
            camera->mClipPlaneFar);
    }

    return cameras;
}

Mesh aiMeshToMesh(const aiMesh* mesh) {
    runtime_assert(mesh != nullptr);
    runtime_assert(mesh->mNormals != nullptr);
    runtime_assert(mesh->mVertices != nullptr);

    Mesh result;

    if (mesh->mBitangents == nullptr ||
        mesh->mTangents == nullptr) {
        result.indices.resize(mesh->mNumFaces * 3);
        result.bitangents.resize(mesh->mNumFaces * 3);
        result.normals.resize(mesh->mNumFaces * 3);
        result.tangents.resize(mesh->mNumFaces * 3);
        result.vertices.resize(mesh->mNumFaces * 3);

        for (size_t j = 0; j < mesh->mNumFaces; ++j) {
            runtime_assert(mesh->mFaces[j].mNumIndices == 3);

            for (size_t k = 0; k < 3; ++k) {
                unsigned index = mesh->mFaces[j].mIndices[k];
                result.indices[j * 3 + k] = j * 3 + k;
                result.normals[j * 3 + k] = toVec3(mesh->mNormals[index]);
                result.vertices[j * 3 + k] = toVec3(mesh->mVertices[index]);
            }

            vec3 edge = result.vertices[j * 3 + 1] - result.vertices[j * 3 + 0];

            for (size_t k = 0; k < 3; ++k) {
                vec3 normal = result.normals[j * 3 + k];
                vec3 tangent = normalize(edge - dot(normal, edge) * normal);
                vec3 bitangent = normalize(cross(normal, tangent));
                result.bitangents[j * 3 + k] = bitangent;
                result.tangents[j * 3 + k] = tangent;
            }
        }
    }
    else {
        result.bitangents.resize(mesh->mNumVertices);
        result.normals.resize(mesh->mNumVertices);
        result.tangents.resize(mesh->mNumVertices);
        result.vertices.resize(mesh->mNumVertices);

        for (size_t j = 0; j < mesh->mNumVertices; ++j) {
            result.bitangents[j] = toVec3(mesh->mBitangents[j]);
            result.normals[j] = toVec3(mesh->mNormals[j]);
            result.tangents[j] = toVec3(mesh->mTangents[j]);
            result.vertices[j] = toVec3(mesh->mVertices[j]);
        }

        result.indices.resize(mesh->mNumFaces * 3);

        for (size_t j = 0; j < mesh->mNumFaces; ++j) {
            runtime_assert(mesh->mFaces[j].mNumIndices == 3);

            for (size_t k = 0; k < 3; ++k) {
                result.indices[j * 3 + k] = mesh->mFaces[j].mIndices[k];
            }
        }
    }

    result.name = mesh->mName.C_Str();
    result.materialID = mesh->mMaterialIndex;

    return result;
}

shared<Scene> loadScene(string path) {
    Assimp::Importer importer;

    auto flags =
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_PreTransformVertices;

    const aiScene* scene = importer.ReadFile(path, flags);

    using namespace std;

    if (!scene) {
        throw std::runtime_error("Cannot load \"" + path + "\" scene.");
    }

    vector<Mesh> meshes;
    vector<size_t> emissive;

    for (size_t i = 0; i < scene->mNumMeshes; ++i) {
        if (isEmissive(scene, i)) {
            emissive.push_back(i);
        }
        else {
            meshes.push_back(aiMeshToMesh(scene->mMeshes[i]));
        }
    }

    AreaLights lights = loadAreaLights(scene);

    Materials materials;

    for (size_t i = 0; i < scene->mNumMaterials; ++i) {
        const aiMaterial* material = scene->mMaterials[i];

        materials.names.push_back(name(scene->mMaterials[i]));
        materials.diffuses.push_back(diffuse(scene->mMaterials[i]));
        materials.speculars.push_back(specular(scene->mMaterials[i]));

        if (property<bool>(material, "$mat.blend.transparency.use")) {
            float ior = property<float>(material, "$mat.blend.transparency.ior");
            auto bsdf = unique<BSDF>(new PerfectTransmissionBSDF(ior, 1.0f));
            materials.bsdfs.push_back(std::move(bsdf));
        }
        else if (property<bool>(material, "$mat.blend.mirror.use")) {
            materials.bsdfs.push_back(unique<BSDF>(new PerfectReflectionBSDF()));
        }
        else {
            materials.bsdfs.push_back(unique<BSDF>(new DiffuseBSDF(materials.diffuses.back())));
        }
    }

    shared<Scene> result = make_shared<Scene>(
        loadCameras(scene),
        move(materials),
        move(meshes),
        move(lights));

    return result;
}

}
