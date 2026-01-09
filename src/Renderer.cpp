#include "Renderer.h"
#include <backends/imgui_impl_opengl3.h>

GLuint Renderer::fbo = 0;
GLuint Renderer::bakedTexture = 0;

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
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, CANVAS_W, CANVAS_H);

    ImDrawList* drawList = new ImDrawList(ImGui::GetDrawListSharedData());
    drawList->_ResetForNewFrame();
    drawList->PushTextureID(ImGui::GetIO().Fonts->TexID);
    drawList->PushClipRect(ImVec2(0, 0), ImVec2((float)CANVAS_W, (float)CANVAS_H));

    for (const auto& s : strokes) {
        if (s.points.size() < 2) continue;
        drawList->AddPolyline(s.points.data(), (int)s.points.size(), s.color, 0, s.thickness);
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