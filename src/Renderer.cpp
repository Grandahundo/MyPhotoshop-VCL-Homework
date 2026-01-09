#include "Renderer.h"
#include <backends/imgui_impl_opengl3.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <iostream>
GLuint Renderer::fbo = 0;
GLuint Renderer::bakedTexture = 0;


GLuint Renderer::texCrayon = 0;
GLuint Renderer::texPencil = 0;
GLuint Renderer::texWatercolor = 0;

#include <filesystem> // C++17 标准库
namespace fs = std::filesystem;

std::map<std::string, GLuint> Renderer::brushTextures;
std::vector<std::string> Renderer::brushNames;

void Renderer::ScanAssets() {
    std::string path = "assets";
    
    // 确保目录存在
    if (!fs::exists(path)) {
        std::cout << "Assets directory not found!" << std::endl;
        return;
    }

    for (const auto& entry : fs::directory_iterator(path)) {
        if (entry.path().extension() == ".png" || entry.path().extension() == ".jpg") {
            std::string filePath = entry.path().string();
            // 获取不带路径和后缀的文件名作为笔刷名 (例如 brush_crayon)
            std::string fileName = entry.path().stem().string(); 
            
            GLuint texID = LoadTexture(filePath.c_str());
            if (texID != 0) {
                brushTextures[fileName] = texID;
                brushNames.push_back(fileName);
                std::cout << "Loaded brush: " << fileName << " (ID: " << texID << ")" << std::endl;
            }
        }
    }
}

GLuint Renderer::LoadTexture(const char* path) {
    int w, h, channels;
    stbi_set_flip_vertically_on_load(false); // 保持和ImGui一致
    unsigned char* data = stbi_load(path, &w, &h, &channels, 4);
    if (!data) return 0;

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    
    // 关键：防止边缘采样溢出
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    return tex;
}

#include<iostream>
void Renderer::Init() {
    glGenTextures(1, &bakedTexture);
    glBindTexture(GL_TEXTURE_2D, bakedTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, CANVAS_W, CANVAS_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bakedTexture, 0);
    
    ClearTexture();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // texCrayon = LoadTexture("assets/brush_crayon.png");
    // texPencil = LoadTexture("assets/brush_pencil.png");
    // texWatercolor = LoadTexture("assets/brush_watercolor.png");
    ScanAssets();
}

void Renderer::ClearTexture() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, CANVAS_W, CANVAS_H);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::PerformBake(std::vector<Stroke>& strokes) {
    if (strokes.empty()) return;
    // 开启混合
    glEnable(GL_BLEND);

    // 设置混合方程式：这是最标准的半透明公式
    // 新颜色 = (新颜色 * 新Alpha) + (旧颜色 * (1 - 新Alpha))
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 如果你想让透明度叠加得更自然（防止透明度丢失），可以使用：
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, CANVAS_W, CANVAS_H);

    ImDrawList* drawList = new ImDrawList(ImGui::GetDrawListSharedData());
    drawList->_ResetForNewFrame();
    drawList->PushTextureID(ImGui::GetIO().Fonts->TexID);
    drawList->PushClipRect(ImVec2(0, 0), ImVec2((float)CANVAS_W, (float)CANVAS_H));

    for (const auto& s : strokes) {
        if (s.points.size() < 2) continue;
        
        // DrawStroke(drawList, s, ImVec2(0, 0));
        RenderStroke(drawList, s, ImVec2(0, 0));
        // drawList->AddPolyline(s.points.data(), (int)s.points.size(), s.color, 0, s.thickness);
    }
    drawList->PopClipRect();
    drawList->PopTextureID();

    ImDrawData drawData;
    drawData.Valid = true;
    drawData.CmdListsCount = 1;
    drawData.CmdLists.push_back(drawList);
    drawData.TotalVtxCount = drawList->VtxBuffer.Size;
    drawData.TotalIdxCount = drawList->IdxBuffer.Size;
    drawData.DisplayPos = ImVec2(0, 0);
    drawData.DisplaySize = ImVec2((float)CANVAS_W, (float)CANVAS_H);
    drawData.FramebufferScale = ImVec2(1, 1);

    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    ImGui_ImplOpenGL3_RenderDrawData(&drawData);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    strokes.clear();
    delete drawList;
}