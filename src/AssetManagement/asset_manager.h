﻿#pragma once
#include <windows.h>
#include <assimp/Importer.hpp>
#include <unordered_set>
#include <filesystem>
#include <iostream>
#include "../Renderer/shader.h"
#include <memory>
#include <map>
#include "assets.h"


struct Dir{
    HANDLE handle;
    OVERLAPPED overlapped;
    FILE_NOTIFY_INFORMATION buffer[2][512];

    std::filesystem::path path;
    std::map<std::filesystem::path, std::shared_ptr<Asset>> assets;

    int bufferIndex = 0;

    bool operator==(const Dir &other) const
    {
        return (path == other.path);
    }
};

class AssetManager{
    // For now the hotloader functionality is run on the main thread, if it turns out to be
    // too inefficiant we can always move it to separate thread
    AssetManager() = default;
    static AssetManager& getInstance(){
        static AssetManager instance;
        return instance;
    }

public:
    AssetManager(AssetManager const&)    = delete;
    void operator=(AssetManager const&)  = delete;

    static void addAsset(std::shared_ptr<Asset> asset){getInstance().x_addAsset(asset);}
    static void removeAsset(const std::string& assetPath){getInstance().x_removeAsset(assetPath);}

    template<typename T>
    static std::shared_ptr<T> getAsset(const std::string& name) {return getInstance().x_getAsset<T>(name);}

    static void addShader(std::shared_ptr<Shader> shader){getInstance().x_addShader(shader);}
    static void removeShader(int id){getInstance().x_removeShader(id);}
    static std::shared_ptr<Shader> getShader(const std::string& name) {return getInstance().x_getShader(name);}

    static Assimp::Importer* getAssimpImporter(){return getInstance().x_getAssimpImporter();}
    static void tryReloadAssets() {getInstance().x_tryReloadAssets();}
    static void checkForChanges() {getInstance().x_checkForChanges();}

private:
    std::clock_t timeFirstChange = 0;
    Assimp::Importer importer;
    Assimp::Importer* x_getAssimpImporter(){return &importer;}

    void x_addAsset(std::shared_ptr<Asset> asset);
    void x_removeAsset(const std::string& assetPath);

    template<typename T>
    std::shared_ptr<T> x_getAsset(const std::string &name)
    {
        if (fileAssets.find(name) == fileAssets.end())
            return nullptr;
        else
        {
            auto asset = std::dynamic_pointer_cast<T>(fileAssets.at(name));
            return asset;
        }
    }

    void x_addShader(std::shared_ptr<Shader> shader);
    void x_removeShader(uint id);
    std::shared_ptr<Shader> x_getShader(const std::string& name);

    void x_tryReloadAssets(); // files may still be locked by application making changes
    void x_checkForChanges(); // run this as often as convieniant

    std::map<std::string, std::shared_ptr<Shader>> shaders;
    std::map<std::filesystem::path, std::shared_ptr<Asset>> fileAssets;
    std::map<std::filesystem::path, std::shared_ptr<Dir>> dirs;
    void addDirWatch(std::shared_ptr<Dir> dir);
    void removeDirWatch(std::shared_ptr<Dir> dir);

};


