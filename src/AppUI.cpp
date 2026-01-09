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

    // 1. 确定纹理 ID
    GLuint texID = 0;
    float spacingFactor = 0.1f; 
    if (Renderer::brushTextures.count(s.brushName)) {
        texID = Renderer::brushTextures[s.brushName];
    }
    // if (s.brushType == BrushType::Crayon)      { texID = Renderer::texCrayon;     spacingFactor = 0.15f; }
    // else if (s.brushType == BrushType::Pencil) { texID = Renderer::texPencil;     spacingFactor = 0.05f; }
    // else if (s.brushType == BrushType::Watercolor) { texID = Renderer::texWatercolor; spacingFactor = 0.3f; }
    // std::cout << s.brushName << std::endl;
    // std::cout << texID << std::endl;
    // 2. 如果是普通笔刷，直接用 ImGui 原生线段（确保至少能看到东西）
    if (texID == 0) {
        std::vector<ImVec2> absPts;
        for (auto p : s.points) absPts.push_back({ p.x + canvasP0.x, p.y + canvasP0.y });
        dl->AddPolyline(absPts.data(), (int)absPts.size(), s.color, 0, s.thickness);
        return;
    }

    // --- 纹理笔刷逻辑 ---
    // 强制开启混合模式，否则透明部分会变黑或不可见
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    

    for (size_t i = 0; i < s.points.size() - 1; i++) {
        ImVec2 p1 = s.points[i];
        ImVec2 p2 = s.points[i+1];

        float dx = p2.x - p1.x;
        float dy = p2.y - p1.y;
        float dist = sqrtf(dx*dx + dy*dy);

        // 防止 stepSize 过小导致死循环，或者过大导致画不出来
        // 至少保证 1 像素一个章
        float stepSize = fmax(1.0f, s.thickness * spacingFactor);

        // 如果两点距离太短，至少画一个章（解决点击不显示问题）
        int numSteps = (dist < stepSize) ? 1 : (int)(dist / stepSize);

        for (int step = 0; step < numSteps; step++) {
            float t = (numSteps == 1) ? 0.0f : (float)step / (float)numSteps;
            
            // 关键：加上画布起始偏移 canvasP0
            ImVec2 stampPos = { 
                p1.x + dx * t + canvasP0.x, 
                p1.y + dy * t + canvasP0.y 
            };

            float r = s.thickness;

            // --- 实现随机旋转 (AddImageQuad) ---
            // 如果 AddImage 不显示，通常是 Quad 的顶点顺序错了
            float angle = (float)(rand() % 360) * (3.14159f / 180.0f);
            float cosA = cosf(angle);
            float sinA = sinf(angle);

            // 定义相对于中心点的四个角
            ImVec2 corners[4] = {
                {-r, -r}, {r, -r}, {r, r}, {-r, r}
            };

            ImVec2 q[4];
            for (int n = 0; n < 4; n++) {
                // 旋转矩阵计算
                float rx = corners[n].x * cosA - corners[n].y * sinA;
                float ry = corners[n].x * sinA + corners[n].y * cosA;
                q[n] = { stampPos.x + rx, stampPos.y + ry };
            }

            // 绘制
            dl->AddImageQuad((ImTextureID)(intptr_t)texID, q[0], q[1], q[2], q[3], 
                             ImVec2(0,0), ImVec2(1,0), ImVec2(1,1), ImVec2(0,1), s.color);
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