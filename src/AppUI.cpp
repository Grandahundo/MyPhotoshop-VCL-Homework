#include "AppUI.h"
#include "Renderer.h"
#include "CanvasLogic.h"

Tool AppUI::currentTool = Tool::Brush;
float AppUI::brushSize = 5.0f;
ImVec4 AppUI::brushColor = {1,0,0,1};
std::vector<Stroke> AppUI::strokes;
bool AppUI::isDrawing = false;
ImVec2 AppUI::rectStartPos = {0,0};

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
    
    ImGui::SliderFloat("Size", &brushSize, 1, 50);
    ImGui::ColorEdit4("Color", (float*)&brushColor);
    
    if (ImGui::Button("Bake", {-1, 40})) Renderer::PerformBake(strokes);
    if (ImGui::Button("Clear All", {-1, 40})) { strokes.clear(); Renderer::ClearTexture(); }
    
    ImGui::End();
}

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
        if (currentTool == Tool::Brush) CanvasLogic::ProcessBrush(strokes, relPos, ImGui::ColorConvertFloat4ToU32(brushColor), brushSize, isDrawing);
        else if (currentTool == Tool::Rectangle) CanvasLogic::ProcessRectangle(strokes, relPos, rectStartPos, ImGui::ColorConvertFloat4ToU32(brushColor), brushSize, isDrawing);
        else if (currentTool == Tool::Circle) CanvasLogic::ProcessCircle(strokes, relPos, rectStartPos, ImGui::ColorConvertFloat4ToU32(brushColor), brushSize, isDrawing);
        else if (currentTool == Tool::StrokeEraser && ImGui::IsMouseDown(0)) CanvasLogic::ProcessStrokeEraser(strokes, relPos, brushSize);
        else if (currentTool == Tool::PreciseEraser && ImGui::IsMouseDown(0)) CanvasLogic::ProcessPreciseEraser(strokes, relPos, brushSize);
    }
    if (ImGui::IsMouseReleased(0)) isDrawing = false;

    // 3. 矢量层渲染
    for (const auto& s : strokes) {
        std::vector<ImVec2> screenPts;
        for (auto p : s.points) screenPts.push_back({p.x + p0.x, p.y + p0.y});
        dl->AddPolyline(screenPts.data(), (int)screenPts.size(), s.color, 0, s.thickness);
    }

    ImGui::End();
    ImGui::PopStyleVar();
}