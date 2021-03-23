﻿#pragma once
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_resize.h"
#include "glm.hpp"
#include <filesystem>
#include <iostream>

using namespace glm;

class Asset{
public:
    std::filesystem::path path;
    virtual void doReload() = 0;
    bool rld = false;

};

class Sprite : public Asset
{
public:
    Sprite() = default;
    Sprite(const std::filesystem::path& path);
    ~Sprite();
    uint16 width  = 0;
    uint16 height = 0;
    u8vec4* data  = nullptr;

public:
    bool loadFromFile(const std::filesystem::path& path);
    virtual void doReload() override;
};

Sprite::Sprite(const std::filesystem::path& path)
{
    loadFromFile(path);
    this->path = path;
}

Sprite::~Sprite()
{
    if(data)
        delete[] data;
}

void Sprite::doReload()
{
    if(loadFromFile(path))
    {
        std::cout<<"Sprite "<<path<<" reloaded.\n";
        rld = false;
    }
}

bool Sprite::loadFromFile(const std::filesystem::path& path){

    //TODO: add error logging
    int w = 0, h = 0, ch = 0;

    if(data != nullptr)
        delete[] data;

    if(!std::filesystem::exists(path))
        return false;

    auto temp = stbi_load(path.string().c_str(), &w, &h, &ch, 4);
    if(!temp)
        return false;
    width = w;
    height = h;

    data = new u8vec4[w * h];
    std::memcpy(data, temp, w * h * 4);
    return true;
}