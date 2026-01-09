#include "CanvasLogic.h"
#include <algorithm>

float CanvasLogic::GetDistance(ImVec2 p1, ImVec2 p2) {
    return std::sqrt(std::pow(p1.x - p2.x, 2) + std::pow(p1.y - p2.y, 2));
}

std::vector<ImVec2> densify(std::vector<ImVec2> s, int iter = 2) {
    std::vector<ImVec2> dense;
    dense.push_back(s[0]);
    for (int i = 1; i < (int)s.size(); i++) {
        ImVec2 lst = s[i - 1];
        ImVec2 cur = s[i];
        for (int j = 1; j < iter; j++) {
            dense.push_back(
                ImVec2(
                    (lst.x * (iter - j) + cur.x * j) / iter,
                    (lst.y * (iter - j) + cur.y * j) / iter
                )
            );
        }
        dense.push_back(cur);
    }
    return dense;
}

void CanvasLogic::ProcessBrush(std::vector<Stroke>& strokes, ImVec2 relPos, ImU32 color, float size, bool& isDrawing) {
    if (ImGui::IsMouseClicked(0)) {
        isDrawing = true;
        strokes.push_back(Stroke({relPos}, color, size));
    }
    if (isDrawing && ImGui::IsMouseDown(0)) {
        if (GetDistance(strokes.back().points.back(), relPos) > 2.0f)
            strokes.back().points.push_back(relPos);
    }
}
// #include <iostream>
void CanvasLogic::ProcessRectangle(std::vector<Stroke>& strokes, ImVec2 relPos, ImVec2& startPos, ImU32 color, float size, bool& isDrawing) {
    if (ImGui::IsMouseClicked(0)) {
        isDrawing = true;
        startPos = relPos;
        std::vector<ImVec2> pts(5, relPos);
        strokes.push_back(Stroke(pts, color, size));
    }
    if (isDrawing && ImGui::IsMouseDown(0)) {
        Stroke& s = strokes.back();
        s.points[1] = { relPos.x, startPos.y };
        s.points[2] = relPos;
        s.points[3] = { startPos.x, relPos.y };
        s.points[4] = startPos;
    }
    if (isDrawing && ImGui::IsMouseReleased(0)) {
        // std::cout << "released" << std::endl;
        Stroke& s = strokes.back();
        s.points.clear();
        std::vector<ImVec2> des;
        des.push_back(startPos);
        des.push_back({relPos.x, startPos.y});
        des.push_back(relPos);
        des.push_back({startPos.x, relPos.y});
        des.push_back(startPos);
        // s.points.push_back(startPos);
        for (int k = 1; k < 5; k++) {
            ImVec2 lst = des[k - 1];
            ImVec2 cur = des[k];
            for (int i = 0; i < 100; i++) {
                ImVec2 mid = {
                    ((100 - i) * lst.x + i * cur.x) / 100,
                    ((100 - i) * lst.y + i * cur.y) / 100
                };
                s.points.push_back(mid);
            }
        }
        s.points.push_back(startPos);

    }
}

// #include <iostream>
void CanvasLogic::ProcessCircle(std::vector<Stroke>& strokes, ImVec2 relPos, ImVec2& startPos, ImU32 color, float size, bool& isDrawing) {
    if (ImGui::IsMouseClicked(0)) {
        isDrawing = true;
        startPos = relPos;
        std::vector<ImVec2> pts(360, relPos);
        strokes.push_back(Stroke(pts, color, size));
    }
    if (isDrawing && ImGui::IsMouseDown(0)) {
        Stroke& s = strokes.back();
        ImVec2 origin((relPos.x + startPos.x) / 2, (relPos.y + startPos.y) / 2);
        float radiusX = abs(relPos.x - startPos.x) / 2;
        float radiusY = abs(relPos.y - startPos.y) / 2;
        for (int i = 0; i < 360; i++) {
            s.points[i] = {
                float(origin.x + radiusX * cos(i / 360.0 * 2.0 * 3.1415926)),
                float(origin.y + radiusY * sin(i / 360.0 * 2.0 * 3.1415926))
            };
        }
        // std::cout << "Drawing" << std::endl;
    }
    if (isDrawing && ImGui::IsMouseReleased(0)) {
        Stroke& s = strokes.back();
        s.points = densify(s.points, 2);
    }
}


void CanvasLogic::ProcessStrokeEraser(std::vector<Stroke>& strokes, ImVec2 relPos, float eraserSize) {
    strokes.erase(std::remove_if(strokes.begin(), strokes.end(), [&](const Stroke& s) {
                    for (const auto& p : s.points) {
                        if (GetDistance(p, relPos) < eraserSize + s.thickness) return true;
                    }
                    return false;
                }), strokes.end());
}

void CanvasLogic::ProcessPreciseEraser(std::vector<Stroke>& strokes, ImVec2 relPos, float eraserSize) {
    std::vector<Stroke> next;
    for (const auto& s : strokes) {
        std::vector<ImVec2> seg;
        for (const auto& p : s.points) {
            if (GetDistance(p, relPos) <= eraserSize + s.thickness) {
                if (seg.size() >= 2) next.push_back({seg, s.color, s.thickness});
                seg.clear();
            } else {
                seg.push_back(p);
            }
        }
        if (seg.size() >= 2) next.push_back({seg, s.color, s.thickness});
    }
    strokes = std::move(next);
}