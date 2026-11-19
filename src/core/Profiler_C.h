#pragma once
// 구간별 시간 측정 (docs/design/measure.md). Spatial Hash 전/후를 같은 장면에서 비교하려고.
// std::chrono만 씀 -> raylib 모름, 테스트 가능.
#include <array>
#include <chrono>
#include <cstddef>

enum class ProfileSection
{
    Tick,           // 고정 틱 전체 (아래 구간 + 나머지)
    Ai,             // EnemyAI + ShooterAI + 발사 의도 -> 탄 생성
    Physics,        // Motor + Physics (+ 잔상)
    Projectiles,    // 탄 이동·벽
    SpatialCombat,  // SpatialQuery.Rebuild + 공격 단계 + 근접·탄 판정 (= 주변 검색을 쓰는 곳)
    Render,         // 그리기 명령 제출 (vsync 대기 제외). 이것만 프레임당 1번, 나머지는 틱당 1번
    Count,
};

inline constexpr std::size_t kProfileSectionCount = static_cast<std::size_t>(ProfileSection::Count);

// 프레임마다: BeginFrame -> (구간 측정 여러 번) -> EndFrame
// 느린 프레임은 틱을 여러 번 돌림 -> "프레임당 합"은 부풀어 보임. 그래서 "1번당(틱당) 평균"도 같이 둠 (11/19 보강)
class FrameProfiler
{
public:
    void BeginFrame();
    void Add(ProfileSection eSection, double dMs);
    void EndFrame();

    // 이번 프레임 (EndFrame 뒤 = 방금 끝난 프레임)
    double GetLastFrameMs(ProfileSection eSection) const { return m_arrLastMs[ToIndex(eSection)]; }
    int GetLastFrameCalls(ProfileSection eSection) const { return m_arrLastCalls[ToIndex(eSection)]; }
    // 화면 표시용 지수 이동 평균 (숫자가 매 프레임 튀어서 못 읽는 것 방지)
    double GetSmoothedMs(ProfileSection eSection) const { return m_arrSmoothedMs[ToIndex(eSection)]; }
    double GetSmoothedPerCallMs(ProfileSection eSection) const { return m_arrSmoothedPerCallMs[ToIndex(eSection)]; }

    static constexpr double kSmoothing = 0.1;  // 새 값 비율 (작을수록 부드러움)

private:
    static std::size_t ToIndex(ProfileSection eSection) { return static_cast<std::size_t>(eSection); }

    std::array<double, kProfileSectionCount> m_arrFrameMs = {};
    std::array<int, kProfileSectionCount> m_arrFrameCalls = {};
    std::array<double, kProfileSectionCount> m_arrLastMs = {};
    std::array<int, kProfileSectionCount> m_arrLastCalls = {};
    std::array<double, kProfileSectionCount> m_arrSmoothedMs = {};
    std::array<double, kProfileSectionCount> m_arrSmoothedPerCallMs = {};
    std::array<bool, kProfileSectionCount> m_arrHasPerCall = {};  // 한 번이라도 불렸나 (첫 값은 평균 없이)
    bool m_bHasSample = false;
};

// RAII 구간 측정: 만들 때 시작, 블록을 나갈 때(소멸자) 걸린 시간을 Add.
// C#이라면 using (new ProfileScope(...)) { } 와 같은 모양
class ProfileScope
{
public:
    ProfileScope(FrameProfiler& tProfiler, ProfileSection eSection)
        : m_tProfiler(tProfiler), m_eSection(eSection), m_tStart(std::chrono::steady_clock::now())
    {
    }
    ~ProfileScope()
    {
        const std::chrono::duration<double, std::milli> tElapsed = std::chrono::steady_clock::now() - m_tStart;
        m_tProfiler.Add(m_eSection, tElapsed.count());
    }
    ProfileScope(const ProfileScope&) = delete;
    ProfileScope& operator=(const ProfileScope&) = delete;

private:
    FrameProfiler& m_tProfiler;
    ProfileSection m_eSection = ProfileSection::Tick;
    std::chrono::steady_clock::time_point m_tStart;
};

const char* GetProfileSectionName(ProfileSection eSection);
