#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <iostream>

// --- 数据结构 ---
struct Stroke {
    std::vector<ImVec2> points; // 存储相对画布左上角的坐标 (0~width, 0~height)
    ImU32 color;
    float thickness;
};

enum class Tool { 
    Brush, 
    StrokeEraser,
    PreciseEraser,
};

// --- 全局变量 ---
std::vector<Stroke> g_strokes;
Tool g_currentTool = Tool::Brush;
float g_brushSize = 5.0f;
ImVec4 g_brushColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
bool g_isDrawing = false;
bool g_pendingBake = false;

// 资源 ID
GLuint g_fbo = 0;
GLuint g_bakedTexture = 0;
const int CANVAS_W = 1280 - 250; // 定义一个固定的高分辨率画布
const int CANVAS_H = 720;

// --- 工具函数 ---
float GetDistance(ImVec2 p1, ImVec2 p2) {
    return std::sqrt(std::pow(p1.x - p2.x, 2) + std::pow(p1.y - p2.y, 2));
}

// --- 初始化 FBO 和 纹理 ---
void InitCanvasResources() {
    glGenTextures(1, &g_bakedTexture);
    glBindTexture(GL_TEXTURE_2D, g_bakedTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, CANVAS_W, CANVAS_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenFramebuffers(1, &g_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_bakedTexture, 0);

    // 初始清屏：将画布设为透明或白色
    glViewport(0, 0, CANVAS_W, CANVAS_H);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // 白色画布
    glClear(GL_COLOR_BUFFER_BIT);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// --- 核心烘焙函数 ---
void PerformBake(std::vector<Stroke>& strokes) {
    if (g_strokes.empty()) return;

    // 1. 绑定 FBO 并设置视口
    glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
    glViewport(0, 0, CANVAS_W, CANVAS_H);

    // 2. 准备绘制列表
    ImDrawList* drawList = new ImDrawList(ImGui::GetDrawListSharedData());
    drawList->_ResetForNewFrame();
    
    // 【关键修复 1】告诉 ImGui 使用字体纹理，以便正确采样“白色像素”来绘制颜色
    ImGuiIO& io = ImGui::GetIO();
    drawList->PushTextureID(io.Fonts->TexID); 
    
    drawList->PushClipRect(ImVec2(0, 0), ImVec2((float)CANVAS_W, (float)CANVAS_H));

    // 渲染所有笔画
    for (const auto& stroke : strokes) {
        if (stroke.points.size() < 2) continue;
        drawList->AddPolyline(stroke.points.data(), (int)stroke.points.size(), stroke.color, 0, stroke.thickness);
    }
    
    drawList->PopClipRect();
    drawList->PopTextureID(); // 对应 PushTextureID

    // 3. 构造渲染数据
    ImDrawData drawData;
    drawData.Valid            = true;
    drawData.CmdListsCount    = 1;
    drawData.CmdLists.push_back(drawList);
    drawData.TotalVtxCount    = drawList->VtxBuffer.Size;
    drawData.TotalIdxCount    = drawList->IdxBuffer.Size;
    drawData.DisplayPos       = ImVec2(0, 0);
    drawData.DisplaySize      = ImVec2((float)CANVAS_W, (float)CANVAS_H);
    drawData.FramebufferScale = ImVec2(1, 1);

    // 【关键修复 2】手动配置 OpenGL 混合状态，防止颜色丢失
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);

    // 执行渲染
    ImGui_ImplOpenGL3_RenderDrawData(&drawData);

    // 4. 清理还原
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    g_strokes.clear(); 
    delete drawList;
    g_pendingBake = false;
}


int main() {
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Professional Paint App", NULL, NULL);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    InitCanvasResources();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // --- UI: Sidebar ---
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(250, 720));
        ImGui::Begin("Tools", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
        if (ImGui::RadioButton("Brush", g_currentTool == Tool::Brush)) g_currentTool = Tool::Brush;
        if (ImGui::RadioButton("StrokeEraser", g_currentTool == Tool::StrokeEraser)) g_currentTool = Tool::StrokeEraser;
        if (ImGui::RadioButton("PreciseEraser", g_currentTool == Tool::PreciseEraser)) g_currentTool = Tool::PreciseEraser;
        ImGui::SliderFloat("Size", &g_brushSize, 1.0f, 50.0f);
        ImGui::ColorEdit4("Color", (float*)&g_brushColor);
        if (ImGui::Button("Bake (Save to Texture)", ImVec2(-1, 40))) g_pendingBake = true;
        if (ImGui::Button("Clear All", ImVec2(-1, 40))) {
            g_strokes.clear();
            glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
            glClearColor(1,1,1,1); glClear(GL_COLOR_BUFFER_BIT);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        ImGui::End();

        // --- UI: Canvas ---
        ImGui::SetNextWindowPos(ImVec2(250, 0));
        ImGui::SetNextWindowSize(ImVec2(1280 - 250, 720));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Canvas", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
        
        ImVec2 canvasP0 = ImGui::GetCursorScreenPos();
        ImVec2 canvasSz = ImGui::GetContentRegionAvail();
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // 1. 先画底片（已烘焙的内容）
        drawList->AddImage(
            (ImTextureID)(intptr_t)g_bakedTexture, 
            canvasP0, 
            ImVec2(canvasP0.x + canvasSz.x, canvasP0.y + canvasSz.y), 
            ImVec2(0, 1), // uv_min: 这里的 1 表示纹理底端作为显示顶端
            ImVec2(1, 0)  // uv_max: 这里的 0 表示纹理顶端作为显示底端
        );
        // drawList->AddImage((ImTextureID)(intptr_t)g_bakedTexture, canvasP0, ImVec2(canvasP0.x + canvasSz.x, canvasP0.y + canvasSz.y));
        // std::cout << canvasP0.x << ' ' << canvasP0.y << std::endl;
        // std::cout << canvasSz.x << ' ' << canvasSz.y << std::endl;
        // 2. 处理交互
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 relMousePos = ImVec2(mousePos.x - canvasP0.x, mousePos.y - canvasP0.y);

        if (ImGui::IsWindowHovered()) {
            if (g_currentTool == Tool::Brush) {
                if (ImGui::IsMouseClicked(0)) {
                    g_isDrawing = true;
                    Stroke s;
                    s.color = ImGui::ColorConvertFloat4ToU32(g_brushColor);
                    s.thickness = g_brushSize;
                    s.points.push_back(relMousePos);
                    g_strokes.push_back(s);
                }
                if (g_isDrawing && ImGui::IsMouseDown(0)) {
                    if (GetDistance(g_strokes.back().points.back(), relMousePos) > 2.0f)
                        g_strokes.back().points.push_back(relMousePos);
                }
            } else if (g_currentTool == Tool::StrokeEraser && ImGui::IsMouseDown(0)) {
                g_strokes.erase(std::remove_if(g_strokes.begin(), g_strokes.end(), [&](const Stroke& s) {
                    for (const auto& p : s.points) {
                        if (GetDistance(p, relMousePos) < g_brushSize) return true;
                    }
                    return false;
                }), g_strokes.end());
            } else if (g_currentTool == Tool::PreciseEraser && ImGui::IsMouseDown(0)) {
                
            }
        }
        if (ImGui::IsMouseReleased(0)) g_isDrawing = false;

        // 3. 画当前还未烘焙的矢量笔画
        for (const auto& stroke : g_strokes) {
            if (stroke.points.size() < 2) continue;
            // 渲染时：相对转绝对
            std::vector<ImVec2> screenPoints;
            for (auto p : stroke.points) screenPoints.push_back(ImVec2(p.x + canvasP0.x, p.y + canvasP0.y));
            drawList->AddPolyline(screenPoints.data(), (int)screenPoints.size(), stroke.color, 0, stroke.thickness);
        }

        // 画预览圆圈
        if (ImGui::IsWindowHovered())
            drawList->AddCircle(mousePos, g_brushSize, IM_COL32(100, 100, 100, 255), 0, 2.0f);

        ImGui::End();
        ImGui::PopStyleVar();

        // --- 渲染 ---
        ImGui::Render();

        // 执行烘焙 (必须在 Render 之后，真正绘制之前)
        if (g_pendingBake) PerformBake(g_strokes);

        int dw, dh;
        glfwGetFramebufferSize(window, &dw, &dh);
        glViewport(0, 0, dw, dh);
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}