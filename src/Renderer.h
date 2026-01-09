#pragma once
#include <glad/glad.h>
#include "Common.h"
#include <algorithm>
#include <map>

class Renderer {
public:
    static GLuint fbo;
    static GLuint bakedTexture;

    static std::map<std::string, GLuint> brushTextures;
    static std::vector<std::string> brushNames;
    static GLuint texCrayon;
    static GLuint texPencil;
    static GLuint texWatercolor;

    static void Init();
    static GLuint LoadTexture(const char* path); // 加载函数
    static void PerformBake(std::vector<Stroke>& strokes);
    static void ClearTexture();
    static void ScanAssets();
};