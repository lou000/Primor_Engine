﻿#include "yamlserialization.h"
#include "../Renderer/renderpipeline.h"
#include <fstream>

#define SERIALIZE_PRIMITIVE(e, k) \
    e << Key << #k << Value << k;

#define DESERIALIZE_PRIMITIVE(node, dest, type)\
    dest = node[#dest].as<type>();

using namespace YAML;

namespace YAML {
template<>
struct convert<glm::vec3>
{
    static Node encode(const glm::vec3& rhs)
    {
        Node node;
        node.push_back(rhs.x);
        node.push_back(rhs.y);
        node.push_back(rhs.z);
        node.SetStyle(EmitterStyle::Flow);
        return node;
    }

    static bool decode(const Node& node, glm::vec3& rhs)
    {
        if (!node.IsSequence() || node.size() != 3)
            return false;

        rhs.x = node[0].as<float>();
        rhs.y = node[1].as<float>();
        rhs.z = node[2].as<float>();
        return true;
    }
};

template<>
struct convert<glm::vec4>
{
    static Node encode(const glm::vec4& rhs)
    {
        Node node;
        node.push_back(rhs.x);
        node.push_back(rhs.y);
        node.push_back(rhs.z);
        node.push_back(rhs.w);
        node.SetStyle(EmitterStyle::Flow);
        return node;
    }

    static bool decode(const Node& node, glm::vec4& rhs)
    {
        if (!node.IsSequence() || node.size() != 4)
            return false;

        rhs.x = node[0].as<float>();
        rhs.y = node[1].as<float>();
        rhs.z = node[2].as<float>();
        rhs.w = node[3].as<float>();
        return true;
    }
};

template<>
struct convert<glm::quat>
{
    static Node encode(const glm::quat& rhs)
    {
        Node node;
        node.push_back(rhs.x);
        node.push_back(rhs.y);
        node.push_back(rhs.z);
        node.push_back(rhs.w);
        node.SetStyle(EmitterStyle::Flow);
        return node;
    }

    static bool decode(const Node& node, glm::quat& rhs)
    {
        if (!node.IsSequence() || node.size() != 4)
            return false;

        rhs.x = node[0].as<float>();
        rhs.y = node[1].as<float>();
        rhs.z = node[2].as<float>();
        rhs.w = node[3].as<float>();
        return true;
    }
};

Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
    return out;
}

Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
    return out;
}

Emitter& operator<<(YAML::Emitter& out, const glm::quat& v)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
    return out;
}

}

bool readFile(std::string* strOut, const std::filesystem::path& path)
{
    if(!std::filesystem::exists(path))
    {
        WARN("Config: File %s doesnt exist.", path.string().c_str());
        return false;
    }

    std::ifstream input_file(path);
    if (!input_file.is_open()) {
        WARN("Config: Couldnt open %s file. Config will revert to default!",
             path.string().c_str());
        return false;
    }
    std::stringstream strStream;
    strStream << input_file.rdbuf();
    *strOut = strStream.str();
    input_file.close();
    return true;
}

bool writeFile(const char* str, const std::filesystem::path& path)
{
    std::ofstream fout(path);
    if (!fout.is_open()) {
        WARN("Config: Couldnt open %s file. Config will not be saved!",
             path.string().c_str());
        return false;
    }
    fout << str;
    fout.close();
    return true;
}

bool Serializer::serializeRenderConfig(const RenderConfig &config, const std::filesystem::path &filepath)
{
    LOG("Config: Serializing render pipeline config to file %s\n",
        filepath.string().c_str());
    Emitter e;
    e << BeginMap;
    SERIALIZE_PRIMITIVE(e, config.enableBloom);
    SERIALIZE_PRIMITIVE(e, config.bloomRadius);
    SERIALIZE_PRIMITIVE(e, config.bloomIntensity);
    SERIALIZE_PRIMITIVE(e, config.bloomTreshold);
    SERIALIZE_PRIMITIVE(e, config.exposure);

    SERIALIZE_PRIMITIVE(e, config.enableCSM);
    SERIALIZE_PRIMITIVE(e, config.shadowCascadeCount);
    SERIALIZE_PRIMITIVE(e, config.cascadeZextra);
    SERIALIZE_PRIMITIVE(e, config.firstCascadeOffset);
    SERIALIZE_PRIMITIVE(e, config.csmResolution);

    SERIALIZE_PRIMITIVE(e, config.enableSSAO);
    SERIALIZE_PRIMITIVE(e, config.ssaoKernelSize);
    SERIALIZE_PRIMITIVE(e, config.blurKernelSize);
    SERIALIZE_PRIMITIVE(e, config.ssaoRadius);
    SERIALIZE_PRIMITIVE(e, config.ssaoBias);
    e << EndMap;
    return writeFile(e.c_str(), filepath);
}

RenderConfig Serializer::deserializeRenderConfig(const std::filesystem::path &filepath)
{
    RenderConfig config;
    std::string in;
    if(!readFile(&in, filepath))
        return config;

    Node data = Load(in);
    DESERIALIZE_PRIMITIVE(data, config.enableBloom,    bool);
    DESERIALIZE_PRIMITIVE(data, config.bloomRadius,    float);
    DESERIALIZE_PRIMITIVE(data, config.bloomIntensity, float);
    DESERIALIZE_PRIMITIVE(data, config.bloomTreshold,  float);
    DESERIALIZE_PRIMITIVE(data, config.exposure,       float);

    DESERIALIZE_PRIMITIVE(data, config.enableCSM,          bool);
    DESERIALIZE_PRIMITIVE(data, config.shadowCascadeCount, int);
    DESERIALIZE_PRIMITIVE(data, config.cascadeZextra,      float);
    DESERIALIZE_PRIMITIVE(data, config.firstCascadeOffset, float);
    DESERIALIZE_PRIMITIVE(data, config.csmResolution,      uint);

    DESERIALIZE_PRIMITIVE(data, config.enableSSAO,     bool);
    DESERIALIZE_PRIMITIVE(data, config.ssaoKernelSize, int);
    DESERIALIZE_PRIMITIVE(data, config.blurKernelSize, int);
    DESERIALIZE_PRIMITIVE(data, config.ssaoRadius,     float);
    DESERIALIZE_PRIMITIVE(data, config.ssaoBias,       float);

    return config;
}

bool Serializer::serializeCamera(Emitter& e, std::shared_ptr<Camera> camera)
{
    e << BeginMap;
    SERIALIZE_PRIMITIVE(e, camera->type);
    SERIALIZE_PRIMITIVE(e, camera->m_pos);
    SERIALIZE_PRIMITIVE(e, camera->m_fov);
    SERIALIZE_PRIMITIVE(e, camera->m_aspectRatio);
    SERIALIZE_PRIMITIVE(e, camera->m_nearClip);
    SERIALIZE_PRIMITIVE(e, camera->m_farClip);
    SERIALIZE_PRIMITIVE(e, camera->isActive);
    SERIALIZE_PRIMITIVE(e, camera->m_focusPoint);
    SERIALIZE_PRIMITIVE(e, camera->m_rotation);
    e << EndMap;
    return true;
}

std::shared_ptr<Camera> Serializer::deserializeCamera(const Node& node)
{
    ASSERT(node["camera->type"]);
    Camera::Type type = (Camera::Type)node["camera->type"].as<uint>();

    float fov = node["camera->m_fov"].as<float>();
    float aspectRatio = node["camera->m_aspectRatio"].as<float>();
    float nearClip = node["camera->m_nearClip"].as<float>();
    float farClip = node["camera->m_farClip"].as<float>();

    std::shared_ptr<Camera> camera;
    switch(type)
    {
    case Camera::Type::Base:
        camera = std::make_shared<Camera>(fov, aspectRatio, nearClip, farClip);
    case Camera::Type::EditorCamera:
        camera = std::make_shared<Camera>(fov, aspectRatio, nearClip, farClip); //TODO: fix after implementing
    case Camera::Type::GameCamera:
        camera = std::make_shared<Camera>(fov, aspectRatio, nearClip, farClip); //TODO: fix after implementing
    }

    DESERIALIZE_PRIMITIVE(node, camera->m_pos, vec3);
    DESERIALIZE_PRIMITIVE(node, camera->isActive, bool);
    DESERIALIZE_PRIMITIVE(node, camera->m_focusPoint, vec3);
    DESERIALIZE_PRIMITIVE(node, camera->m_rotation, quat);

    return std::static_pointer_cast<Camera>(camera);
}

bool Serializer::serializeEntity(YAML::Emitter &e, std::shared_ptr<Entity> entity)
{
//    e << BeginMap;
//    SERIALIZE_PRIMITIVE(e, entity->type);
//    SERIALIZE_PRIMITIVE(e, camera->m_pos);
//    SERIALIZE_PRIMITIVE(e, camera->m_fov);
//    SERIALIZE_PRIMITIVE(e, camera->m_aspectRatio);
//    SERIALIZE_PRIMITIVE(e, camera->m_nearClip);
//    SERIALIZE_PRIMITIVE(e, camera->m_farClip);
//    SERIALIZE_PRIMITIVE(e, camera->isActive);
//    SERIALIZE_PRIMITIVE(e, camera->m_focusPoint);
//    SERIALIZE_PRIMITIVE(e, camera->m_rotation);
//    e << EndMap;
    return true;
}

std::shared_ptr<Entity> Serializer::deserializeEntity(const YAML::Node &node)
{
    return nullptr;
}

bool Serializer::serializeDirLight(Emitter& e, DirectionalLight dirLight)
{
    e << BeginMap;
    SERIALIZE_PRIMITIVE(e, dirLight.direction);
    SERIALIZE_PRIMITIVE(e, dirLight.color);
    SERIALIZE_PRIMITIVE(e, dirLight.enabled);
    SERIALIZE_PRIMITIVE(e, dirLight.intensity);
    e << EndMap;
    return false;
}

DirectionalLight Serializer::deserializeDirLight(const Node& node)
{
    DirectionalLight dirLight;
    DESERIALIZE_PRIMITIVE(node, dirLight.direction, vec3);
    DESERIALIZE_PRIMITIVE(node, dirLight.color,     vec3);
    DESERIALIZE_PRIMITIVE(node, dirLight.enabled,   bool);
    DESERIALIZE_PRIMITIVE(node, dirLight.intensity, float);
    return dirLight;
}

bool Serializer::serializeScene(Scene* scene, const std::filesystem::path &filepath)
{
    //TODO: serialize entities!
    LOG("Config: Serializing scene to file %s\n",
        filepath.string().c_str());
    Emitter e;
    e << BeginMap;
    e << Key << "directionalLight"<< Value;
    serializeDirLight(e, scene->directionalLight);

    e << Key << "m_activeCamera"<<Value << (scene->m_activeCamera == scene->m_editorCamera);

    e << Key << "m_gameCamera"<<Value;
    serializeCamera(e, scene->m_gameCamera);

    e << Key << "m_editorCamera"<<Value;
    serializeCamera(e, scene->m_editorCamera);
    e << EndMap;
    return writeFile(e.c_str(), filepath);
    return false;
}

bool Serializer::deserializeScene(Scene *scene, const std::filesystem::path &filepath)
{
    std::string in;
    if(!readFile(&in, filepath))
        return false;

    Node node = Load(in);
    scene->directionalLight = deserializeDirLight(node["directionalLight"]);
    scene->m_gameCamera = deserializeCamera(node["m_gameCamera"]);
    scene->m_editorCamera = deserializeCamera(node["m_editorCamera"]);
    if(node["m_activeCamera"].as<bool>())
        scene->m_activeCamera = scene->m_editorCamera;
    return true;
}
