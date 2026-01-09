#include "Common.h"
    
void DrawStroke(ImDrawList* dl, const Stroke& s, ImVec2 p0) {
    if (s.points.size() < 2) return;

    if (!s.isQuality) {
        // 普通笔刷：原有逻辑
        std::vector<ImVec2> screenPts;
        for (auto p : s.points) screenPts.push_back({p.x + p0.x, p.y + p0.y});
        dl->AddPolyline(screenPts.data(), (int)screenPts.size(), s.color, 0, s.thickness);
    } 
    else {
        // 高质量笔刷：样条插值 + 软边缘
        for (size_t i = 0; i < s.points.size() - 1; i++) {
            // 获取样条曲线所需的 4 个控制点
            ImVec2 p1 = s.points[i];
            ImVec2 p2 = s.points[i+1];
            ImVec2 p0_pt = (i == 0) ? p1 : s.points[i-1];
            ImVec2 p3 = (i + 2 >= s.points.size()) ? p2 : s.points[i+2];

            // 在每两个原始点之间插值出 10 个平滑点
            for (int step = 0; step <= 10; step++) {
                float t = (float)step / 10.0f;
                ImVec2 localPos = InterpolateCatmullRom(p0_pt, p1, p2, p3, t);
                ImVec2 worldPos = {localPos.x + p0.x, localPos.y + p0.y};

                // 核心技巧：画多个圆圈叠加形成羽化效果
                // 1. 底色圆（外层，透明度低）
                dl->AddCircleFilled(worldPos, s.thickness * 1.2f, s.color & 0x33FFFFFF); 
                // 2. 核心圆（内层，透明度高）
                dl->AddCircleFilled(worldPos, s.thickness * 0.8f, s.color);
            }
        }
    }
}

ImVec2 InterpolateCatmullRom(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    return ImVec2(
        0.5f * ((2.0f * p1.x) + (-p0.x + p2.x) * t + (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 + (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3),
        0.5f * ((2.0f * p1.y) + (-p0.y + p2.y) * t + (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 + (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3)
    );
}