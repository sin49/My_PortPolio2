#pragma once
// 자동 측정 시나리오 (Main.exe --bench, docs/design/measure.md 6장).
// 사람이 키를 누르는 대신 같은 순서·같은 시간으로 -> 해시 전/후를 똑같이 재현.
// 타이머 + 상태 enum (코루틴 대신). 명령만 내고 실제 실행은 Game이 함 (raylib 모름 -> 테스트 가능)
#include <string_view>
#include <vector>

struct BenchConfig
{
    // 단계별 적 수. 8000·9900은 렌더 개선 뒤 6000이 60FPS에 붙어서 추가 (World 용량 9999)
    std::vector<int> vecCounts = {500, 1000, 2000, 3000, 4000, 5000, 6000, 8000, 9900};
    double dSpreadDelaySec = 0.5;    // 소환 직후 이만큼 뒤 "퍼짐" 측정 시작 (소환 프레임 튐 제외)
    double dClusterDelaySec = 12.0;  // 소환 후 이만큼 뒤 "몰림" 측정 시작 (Chaser가 플레이어 쪽에 모임)
    double dRecordSec = 3.0;         // 측정 구간 길이
};

// 이번 프레임에 Game이 할 일
struct BenchCommand
{
    bool bResetGame = false;  // 새 게임 (튜토리얼 방, 적 초기화) + 디버그 모드(무적)
    int iSpawnCount = 0;      // 작은 Chaser 소환 수
    bool bStartRecord = false;
    std::string_view svScene;  // "spread" / "cluster"
    int iTarget = 0;           // 기록에 남길 목표 적 수
    double dRecordSec = 3.0;   // 측정 구간 길이
    bool bFinished = false;    // 끝 -> 창 닫기
};

class BenchScript
{
public:
    explicit BenchScript(BenchConfig tConfig = {});

    // 매 프레임. dFrameSec: 실제 프레임 시간, bRecorderBusy: 측정 구간이 아직 안 끝남
    BenchCommand Update(double dFrameSec, bool bRecorderBusy);
    bool IsFinished() const { return m_eStep == BenchStep::Done; }
    int GetStepIndex() const { return static_cast<int>(m_uLevel); }  // 진행 표시용
    int GetStepCount() const { return static_cast<int>(m_tConfig.vecCounts.size()); }

private:
    enum class BenchStep
    {
        Reset,          // 새 게임
        Spawn,          // 다음 프레임에 소환 (새 게임이 한 틱 돈 뒤)
        WaitSpread,
        RecordSpread,
        WaitCluster,
        RecordCluster,
        Done,
    };

    BenchConfig m_tConfig;
    BenchStep m_eStep = BenchStep::Reset;
    std::size_t m_uLevel = 0;
    double m_dSinceSpawnSec = 0.0;
};
