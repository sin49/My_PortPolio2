#pragma once

// 고정 타임스텝 누적기 (Unity FixedUpdate를 직접 구현한 것).
// raylib에 의존하지 않음 -> tests 타깃에서 단독 테스트 가능.
//
// 사용법 (매 렌더 프레임):
//   int iSteps = tTimestep.Advance(fFrameTime);
//   for (int i = 0; i < iSteps; ++i) FixedUpdate(FixedTimestep::kStep);
class FixedTimestep
{
public:
    static constexpr float kStep = 1.0f / 60.0f;
    // 한 프레임이 너무 길면(디버거 정지, 창 드래그 등) 잘라냄.
    // 안 자르면 밀린 업데이트를 따라잡느라 다음 프레임이 또 길어지는 악순환이 생김.
    static constexpr float kMaxFrameTime = 0.25f;

    // 프레임 시간을 누적하고, 이번 프레임에 돌려야 할 고정 업데이트 횟수를 반환.
    int Advance(float fFrameTime)
    {
        if (fFrameTime < 0.0f) fFrameTime = 0.0f;
        if (fFrameTime > kMaxFrameTime) fFrameTime = kMaxFrameTime;

        m_fAccumulator += fFrameTime;

        int iSteps = 0;
        while (m_fAccumulator >= kStep)
        {
            m_fAccumulator -= kStep;
            ++iSteps;
        }
        return iSteps;
    }

    // 다음 고정 틱까지 진행 비율 [0, 1). 나중에 렌더 보간에 사용.
    float GetAlpha() const { return m_fAccumulator / kStep; }

private:
    float m_fAccumulator = 0.0f;
};
