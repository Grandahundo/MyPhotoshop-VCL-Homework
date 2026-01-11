#include "AppUI.h"
#include "Renderer.h"
#include "CanvasLogic.h"
#include <iostream>
#include <string>

Tool AppUI::currentTool = Tool::Brush;
BrushType AppUI::brushType = BrushType::Solid;
std::string brushName;
float AppUI::brushSize = 5.0f;
ImVec4 AppUI::brushColor = {1,0,0,1};
std::vector<Stroke> AppUI::strokes;
bool AppUI::isDrawing = false;
ImVec2 AppUI::rectStartPos = {0,0};

void RenderStroke(ImDrawList* dl, const Stroke& s, ImVec2 canvasP0) {
    if (s.points.size() < 2) return;
    srand(s.id);

    GLuint texID = 0;
    if (Renderer::brushTextures.count(s.brushName)) 
        texID = Renderer::brushTextures[s.brushName];

    // 特殊配置参数
    float spacing = 0.1f;
    float jitterPos = 0.0f;
    float jitterSize = 0.0f;
    bool randomRotation = true;

    // 根据笔刷名字设置特殊表现
    if (s.brushName.find("star") != std::string::npos) {
        spacing = 0.8f;      // 星星要稀疏一点，才像撒出来的
        jitterPos = 2.0f;    // 坐标乱跳
        jitterSize = 0.5f;   // 大小不一
    } else if (s.brushName.find("smoke") != std::string::npos) {
        spacing = 0.2f;
        jitterSize = 1.2f;   // 烟雾忽大忽小
    } else if (s.brushName.find("grass") != std::string::npos) {
        spacing = 0.05f;
        randomRotation = false; // 草丛通常随鼠标方向，而不是乱转
    } else if (s.brushName.find("glitch") != std::string::npos) {
        spacing = 0.3f;
        jitterPos = 1.5f;
    }

    for (size_t i = 0; i < s.points.size() - 1; i++) {
        ImVec2 p1 = s.points[i];
        ImVec2 p2 = s.points[i+1];
        float dist = CanvasLogic::GetDistance(p1, p2);
        float step = fmax(1.0f, s.thickness * spacing);

        for (float d = 0; d < dist; d += step) {
            float t = d / dist;
            ImVec2 basePos = { p1.x + (p2.x-p1.x)*t + canvasP0.x, p1.y + (p2.y-p1.y)*t + canvasP0.y };

            // 1. 坐标抖动
            float px = basePos.x + (rand()%100 - 50)/50.0f * s.thickness * jitterPos;
            float py = basePos.y + (rand()%100 - 50)/50.0f * s.thickness * jitterPos;

            // 2. 随机大小
            float currentSize = s.thickness * (1.0f + (rand()%100 - 50)/50.0f * jitterSize);

            // 3. 旋转逻辑
            float angle = 0;
            if (randomRotation) {
                angle = (rand() % 360) * (3.1415f / 180.0f);
            } else {
                // 如果不随机旋转，就跟随鼠标移动方向
                angle = atan2f(p2.y - p1.y, p2.x - p1.x);
            }

            // 4. 准备绘制顶点
            float cosA = cosf(angle); float sinA = sinf(angle);
            ImVec2 corners[4] = {{-currentSize,-currentSize}, {currentSize,-currentSize}, {currentSize,currentSize}, {-currentSize,currentSize}};
            ImVec2 q[4];
            for (int n=0; n<4; n++) {
                q[n].x = px + (corners[n].x * cosA - corners[n].y * sinA);
                q[n].y = py + (corners[n].x * sinA + corners[n].y * cosA);
            }

            dl->AddImageQuad((ImTextureID)(intptr_t)texID, q[0], q[1], q[2], q[3], 
                             {0,0}, {1,0}, {1,1}, {0,1}, s.color);
        }
    }
}

void AppUI::Render(bool& shouldBake) {
    Sidebar();
    Canvas();
    if (shouldBake) {
        Renderer::PerformBake(strokes);
        shouldBake = false;
    }
}

void AppUI::Sidebar() {
    ImGui::SetNextWindowPos({0,0});
    ImGui::SetNextWindowSize({250, 720});
    ImGui::Begin("Tools", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
    
    if (ImGui::RadioButton("Brush", currentTool == Tool::Brush)) currentTool = Tool::Brush;
    if (ImGui::RadioButton("Rectangle", currentTool == Tool::Rectangle)) currentTool = Tool::Rectangle;
    if (ImGui::RadioButton("Circle", currentTool == Tool::Circle)) currentTool = Tool::Circle;
    if (ImGui::RadioButton("StrokeEraser", currentTool == Tool::StrokeEraser)) currentTool = Tool::StrokeEraser;
    if (ImGui::RadioButton("PreciseEraser", currentTool == Tool::PreciseEraser)) currentTool = Tool::PreciseEraser;
    if (ImGui::Button("test")) {
        for (int i = 0; i < Renderer::brushNames.size(); i++) {
            ImU32 color = IM_COL32(rand() % 255, rand() % 255, rand() % 255, rand() % 255); // 使用宏创建，最安全
            std::vector<ImVec2> points;
            for (int j = 0; j < 100; j++)
                points.push_back(ImVec2(j * 20, i * 50 + 50));
            AppUI::strokes.emplace_back(
                points, color, 15, Renderer::brushNames[i]
            );
        }
    }
    
    static int currentBrushIdx = 0;
    
    if (!Renderer::brushNames.empty()) {
        // ImGui 的 Combo 需要处理字符串数组，我们可以用一个 lambda 表达式
        if (ImGui::BeginCombo("Brush Style", Renderer::brushNames[currentBrushIdx].c_str())) {
            for (int n = 0; n < Renderer::brushNames.size(); n++) {
                const bool is_selected = (currentBrushIdx == n);
                if (ImGui::Selectable(Renderer::brushNames[n].c_str(), is_selected))
                    currentBrushIdx = n;
                
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    } else {
        ImGui::Text("No brushes found in /assets");
    }
    brushName = Renderer::brushNames[currentBrushIdx];

    // 在 AppUI::Sidebar() 中
    ImGui::Text("Brush Settings");
    ImGui::SliderFloat("Size", &brushSize, 1.0f, 50.0f);

    // 重点：开启 AlphaBar 标记，这样取色器右侧会出现透明度滑条
    ImGui::ColorEdit4("Color", (float*)&brushColor, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview);

    // 或者单独加一个不透明度滑块 (Opacity)
    // ImGui::SliderFloat("Opacity", &brushColor.w, 0.0f, 1.0f);
    // ImGui::SliderFloat("Size", &brushSize, 1, 50);
    // ImGui::ColorEdit4("Color", (float*)&brushColor);
    
    if (ImGui::Button("Bake", {-1, 40})) Renderer::PerformBake(strokes);
    if (ImGui::Button("Clear All", {-1, 40})) { strokes.clear(); Renderer::ClearTexture(); }
    
    ImGui::End();
}

#include<iostream>
void AppUI::Canvas() {
    ImGui::SetNextWindowPos({250, 0});
    ImGui::SetNextWindowSize({CANVAS_W, CANVAS_H});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0,0});
    ImGui::Begin("Canvas", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
    
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 mousePos = ImGui::GetMousePos();
    ImVec2 relPos = {mousePos.x - p0.x, mousePos.y - p0.y};
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // 1. 底图
    dl->AddImage((ImTextureID)(intptr_t)Renderer::bakedTexture, p0, {p0.x + CANVAS_W, p0.y + CANVAS_H}, {0,1}, {1,0});

    // 2. 交互
    if (ImGui::IsWindowHovered()) {
        if (currentTool == Tool::Brush) CanvasLogic::ProcessBrush(strokes, relPos, ImGui::ColorConvertFloat4ToU32(brushColor), brushSize, isDrawing, brushName);
        else if (currentTool == Tool::Rectangle) CanvasLogic::ProcessRectangle(strokes, relPos, rectStartPos, ImGui::ColorConvertFloat4ToU32(brushColor), brushSize, isDrawing);
        else if (currentTool == Tool::Circle) CanvasLogic::ProcessCircle(strokes, relPos, rectStartPos, ImGui::ColorConvertFloat4ToU32(brushColor), brushSize, isDrawing);
        else if (currentTool == Tool::StrokeEraser && ImGui::IsMouseDown(0)) CanvasLogic::ProcessStrokeEraser(strokes, relPos, brushSize);
        else if (currentTool == Tool::PreciseEraser && ImGui::IsMouseDown(0)) CanvasLogic::ProcessPreciseEraser(strokes, relPos, brushSize);
    }
    if (ImGui::IsMouseReleased(0)) isDrawing = false;

    // 3. 矢量层渲染
    for (const auto& s : strokes) {
        RenderStroke(dl, s, p0);
    }

    ImGui::End();
    ImGui::PopStyleVar();
}