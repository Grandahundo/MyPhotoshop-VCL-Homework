#pragma once
#include <glad/glad.h>
#include "Common.h"

class Renderer {
public:
    static GLuint fbo;
    static GLuint bakedTexture;

    static void Init();
    static void PerformBake(std::vector<Stroke>& strokes);
    static void ClearTexture();
};