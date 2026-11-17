#pragma once

// 축 정렬 사각형 (방 좌표, 왼쪽 위 기준, px)
struct Rect
{
    float fX = 0.0f;
    float fY = 0.0f;
    float fW = 0.0f;
    float fH = 0.0f;
};

// 겹침 판정. 변이 딱 맞닿기만 한 경우는 겹침 아님.
inline bool Overlaps(const Rect& rcA, const Rect& rcB)
{
    return rcA.fX < rcB.fX + rcB.fW && rcB.fX < rcA.fX + rcA.fW && rcA.fY < rcB.fY + rcB.fH && rcB.fY < rcA.fY + rcA.fH;
}
