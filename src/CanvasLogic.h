#pragma once
#include "Common.h"

class CanvasLogic {
public:
    static float GetDistance(ImVec2 p1, ImVec2 p2);
    static void ProcessBrush(std::vector<Stroke>& strokes, ImVec2 relPos, ImU32 color, float size, bool& isDrawing);
    static void ProcessRectangle(std::vector<Stroke>& strokes, ImVec2 relPos, ImVec2& startPos, ImU32 color, float size, bool& isDrawing);
    static void ProcessCircle(std::vector<Stroke>& strokes, ImVec2 relPos, ImVec2& startPos, ImU32 color, float size, bool& isDrawing);
    static void ProcessStrokeEraser(std::vector<Stroke>& strokes, ImVec2 relPos, float eraserSize);
    static void ProcessPreciseEraser(std::vector<Stroke>& strokes, ImVec2 relPos, float eraserSize);
};