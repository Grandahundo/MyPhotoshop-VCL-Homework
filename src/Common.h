#pragma once
#include <imgui.h>
#include <vector>
#include <cmath>

enum class Tool { Brush, StrokeEraser, PreciseEraser, Rectangle, Circle };

struct Stroke {
    std::vector<ImVec2> points; // 相对坐标
    ImU32 color;
    float thickness;
    Stroke() : color(0), thickness(1.0f) {}
    Stroke(std::vector<ImVec2> p, ImU32 c, float t) : points(p), color(c), thickness(t) {}
};

const int CANVAS_W = 1030; // 1280 - 250
const int CANVAS_H = 720;