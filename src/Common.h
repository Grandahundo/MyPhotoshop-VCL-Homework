#pragma once
#include <glad/glad.h> 
#include <imgui.h>
#include <vector>
#include <cmath>
#include <string>

enum class BrushType { Solid, Crayon, Pencil, Watercolor };
enum class Tool { Brush, StrokeEraser, PreciseEraser, Rectangle, Circle };

static int count = 0;
struct Stroke {
    std::vector<ImVec2> points;
    ImU32 color;
    float thickness;
    std::string brushName; // 改为存储名字，不再用枚举
    int id;

    Stroke(std::vector<ImVec2> p, ImU32 c, float t, std::string name) 
        : points(p), color(c), thickness(t), brushName(name) {id = count++;}
};

const int CANVAS_W = 1030; // 1280 - 250
const int CANVAS_H = 720;

// void DrawStroke(ImDrawList* dl, const Stroke& s, ImVec2 p0);
ImVec2 InterpolateCatmullRom(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3, float t);
void RenderStroke(ImDrawList* dl, const Stroke& s, ImVec2 canvasP0);