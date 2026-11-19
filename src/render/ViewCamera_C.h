#pragma once

#include "core/Vec2_C.h"

#include <algorithm>
#include <cmath>

// 화면이 보는 위치 (연출 전용, 판정 무관). Game이 렌더 프레임마다 갱신, Render는 읽기만.
// vTopLeft = 화면 왼쪽 위가 가리키는 방 좌표(px)
class ViewCamera
{
public:
    static constexpr float kFollowSharpness = 10.0f;  // 클수록 빨리 따라감 (프레임레이트 무관 지수 감쇠)

    // vFocus: 따라갈 점(플레이어 중심), vView: 화면에 보이는 방 영역 크기, vRoom: 방 전체 크기
    void Update(Vec2 vFocus, Vec2 vView, Vec2 vRoom, float fFrameDt)
    {
        const Vec2 vTarget = Clamp({vFocus.fX - vView.fX * 0.5f, vFocus.fY - vView.fY * 0.5f}, vView, vRoom);
        const float fT = 1.0f - std::exp(-kFollowSharpness * fFrameDt);
        m_vTopLeft.fX += (vTarget.fX - m_vTopLeft.fX) * fT;
        m_vTopLeft.fY += (vTarget.fY - m_vTopLeft.fY) * fT;
    }

    // 방 전환·재시작 때 부드럽게 따라가지 말고 바로 이동
    void SnapTo(Vec2 vFocus, Vec2 vView, Vec2 vRoom)
    {
        m_vTopLeft = Clamp({vFocus.fX - vView.fX * 0.5f, vFocus.fY - vView.fY * 0.5f}, vView, vRoom);
    }

    Vec2 GetTopLeft() const { return m_vTopLeft; }

    // ---- 화면 흔들림 (11/19, 플레이어 피격감) ----
    // "트라우마" 방식: 0~1 값을 더하고 시간이 지나면 줄어듦. 흔들림 크기 = 트라우마² (작은 충격은 약하게, 큰 충격은 확)
    static constexpr float kMaxShakePx = 12.0f;   // 트라우마 1일 때 최대 흔들림
    static constexpr float kShakeDecay = 2.2f;    // 초당 줄어드는 트라우마

    void AddShake(float fTrauma) { m_fTrauma = std::min(1.0f, m_fTrauma + fTrauma); }
    void UpdateShake(float fFrameDt)
    {
        m_fTrauma = std::max(0.0f, m_fTrauma - kShakeDecay * fFrameDt);
        m_fShakeTime += fFrameDt;
    }
    // 그리기용 오프셋 (Render만 더함. 미니맵·컬링·화살표는 흔들리지 않는 GetTopLeft 사용)
    // 랜덤 대신 주파수가 다른 sin 두 개 -> 상태 없이 매끄럽고, 같은 시간이면 같은 값
    Vec2 GetShakeOffset() const
    {
        const float fAmount = m_fTrauma * m_fTrauma * kMaxShakePx;
        return Vec2{fAmount * std::sin(m_fShakeTime * 53.0f), fAmount * std::sin(m_fShakeTime * 71.0f + 1.3f)};
    }

private:
    // 방 밖을 보여 주지 않게. 방이 화면보다 작으면 가운데 정렬
    static Vec2 Clamp(Vec2 vPos, Vec2 vView, Vec2 vRoom)
    {
        auto fnAxis = [](float fPos, float fView, float fRoom) {
            if (fRoom <= fView)
                return (fRoom - fView) * 0.5f;
            return std::clamp(fPos, 0.0f, fRoom - fView);
        };
        return Vec2{fnAxis(vPos.fX, vView.fX, vRoom.fX), fnAxis(vPos.fY, vView.fY, vRoom.fY)};
    }

    Vec2 m_vTopLeft = {};
    float m_fTrauma = 0.0f;
    float m_fShakeTime = 0.0f;
};
