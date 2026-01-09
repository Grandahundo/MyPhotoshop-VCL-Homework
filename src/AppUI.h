#pragma once
#include "Common.h"

class AppUI {
public:
    static void Render(bool& shouldBake);
private:
    static void Sidebar();
    static void Canvas();
    
    static Tool currentTool;
    static float brushSize;
    static ImVec4 brushColor;
    static std::vector<Stroke> strokes;
    static bool isDrawing;
    static ImVec2 rectStartPos;
};