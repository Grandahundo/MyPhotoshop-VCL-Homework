#pragma once
#include <imgui.h>
#include <vector>
#include <cmath>

enum class Tool { Brush, StrokeEraser, PreciseEraser, Rectangle, Circle, QualityBrush };

struct Stroke {
    std::vector<ImVec2> points; // 相对坐标
    ImU32 color;
    float thickness;
    bool isQuality; 

    Stroke() : color(0), thickness(1.0f), isQuality(false) {}
    Stroke(std::vector<ImVec2> p, ImU32 c, float t, bool q = false) : points(p), color(c), thickness(t), isQuality(q) {}
};

const int CANVAS_W = 1030; // 1280 - 250
const int CANVAS_H = 720;

void DrawStroke(ImDrawList* dl, const Stroke& s, ImVec2 p0);
ImVec2 InterpolateCatmullRom(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3, float t);