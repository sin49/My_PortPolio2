# C++ 2D 룸 클리어 액션 (포트폴리오 프로토타입)

**이름: (미정)**

> 이 정리글은 AI(Claude)를 이용해 작성했습니다. 내용 확인과 최종 수정은 개발자가 했습니다.
>
> **개발 기간: 4일 (2026-11-16 ~ 11-19)**. AI 협업으로 짧은 기간에 "플레이 가능 + 성능 측정 근거"까지 만드는 것을 목표로 함.
>
> **그래픽·사운드 리소스는 아직 적용하지 않았음.** 캐릭터·적·맵은 색 사각형이고, 효과음은 파일 없이 코드로 합성한 소리. 구조와 성능 검증을 먼저 하고, 리소스는 다음 단계에서 붙일 예정.
>
> **실행**: `Main.exe` 하나 (Windows, 리소스 파일 없이 실행) · C++ 약 8,000줄

## 1. 장르 · 구성
- **장르**: 2D 횡스크롤 근접 액션 + 룸 클리어 (로그라이트 구조의 프로토타입)
- **구성**: 튜토리얼 → 방 1 → 방 2 → 보스방
  - 방에 들어가면 문이 잠기고, 적을 모두 잡으면 문이 열리고 아이템 1개가 떨어짐
  - 적 3종: 근접 Chaser(크기 3종), 원거리 Shooter, 2페이즈 보스
  - 아이템 5종: 겹쳐 먹을수록 스탯이 오름 (값을 덮어쓰지 않고 기본값 + 보정값으로 다시 계산)

## 2. 플레이 방식
| 키 | 동작 |
|---|---|
| ← → | 이동 |
| Z | 점프 (길게 누르면 높이) |
| X | 공격 |
| Shift | 짧게: 대시(무적) / 누르고 있기: 달리기 |
| F1 | 디버그 패널 + 무적 |
| F2 / F4 | 적 1마리 / 100마리 추가 (스트레스 테스트) |

- 적의 공격 직전 깜빡임과 공격 범위 표시를 보고 피하거나 대시로 빠져나간 뒤 반격
- 맞으면 0.6초 무적, 히트스톱, 화면 흔들림

## 3. 기술 스택
**C++20, raylib, CMake + Visual Studio 2022**

### ① 엔티티 풀 + 세대 번호 핸들 — 의사코드를 보고 직접 구현
적을 포인터로 들고 있으면, 적이 죽은 뒤 그 자리에 새 적이 들어와도 모르고 접근하는 문제가 생김.
→ 외부는 `Handle{칸 번호, 세대}`만 들고, 칸이 비워질 때마다 세대를 올려서 옛 핸들을 자동으로 무효화.

```cpp
Entity* World::Get(Handle hTarget)
{
    if (hTarget.uIndex >= kCapacity)
        return nullptr;
    Slot& tSlot = m_vecSlots[hTarget.uIndex];
    return (tSlot.bAlive && tSlot.uGen == hTarget.uGen) ? &tSlot.tValue : nullptr;  // 세대가 다르면 이미 죽은 적
}

void World::FlushRemovals()  // 순회 중에는 삭제 "예약"만, 프레임 끝에 한꺼번에 삭제
{
    for (const std::uint32_t uIndex : m_vecPendingList)
    {
        Slot& tSlot = m_vecSlots[uIndex];
        tSlot.bAlive = false;
        tSlot.uGen += 1;              // 옛 핸들 전부 무효화
        m_vecFreeList.push_back(uIndex);  // 빈 칸 재사용
    }
    m_vecPendingList.clear();
}
```

### ② Spatial Hash (격자 검색) — 의사코드를 보고 직접 구현
충돌 판정 때 "모든 적 × 모든 적"을 비교하면 적 수의 제곱으로 느려짐.
→ 맵을 128px 칸으로 나눠서, 내 근처 칸에 있는 적만 후보로 꺼냄.

```cpp
void SpatialHash::Insert(Handle hEntity, const Rect& rcBounds)
{
    // 몸이 걸친 칸마다 등록 (칸 좌표 2개를 64비트 키 하나로)
    for (int iY = ToCell(rcBounds.fY); iY <= ToCell(rcBounds.fY + rcBounds.fH); ++iY)
        for (int iX = ToCell(rcBounds.fX); iX <= ToCell(rcBounds.fX + rcBounds.fW); ++iX)
            m_mapCells[MakeKey(iX, iY)].push_back(hEntity);
}
// Query: 검색 영역이 걸친 칸들의 후보를 모은 뒤 정렬 + 중복 제거 (여러 칸에 걸친 적)
```

**결과 (CSV로 측정, 같은 PC · Release)**

| 6,000마리 몰림 기준 | FPS | 틱당 로직 |
|---|---|---|
| 전체 순회 (처음) | 25 | 7.04 ms |
| + Spatial Hash | 36.3 | 3.02 ms (2.3배 빨라짐) |
| + 미니맵·화면 밖 그리기 생략 | 60 (상한) | 3.58 ms |
| → **9,900마리** | **58.7** | 8.05 ms |

- 칸 크기 실험(32/64/128/256)도 함: 128이 가장 좋았고, 32는 오히려 전체 순회보다 느림 → 원인은 적이 한 자리에 겹쳐 쌓이는 것
- **측정 도구도 Claude로 만들어서 측정이 편했음**: 게임 중 F5 한 번 = 3초 평균 스냅샷을 CSV 한 줄로 저장, `Main.exe --bench` = 적 수를 500 → 9,900마리까지 자동으로 늘리며 측정해 CSV로 저장. 사람이 키를 누르는 것보다 재현성이 높고, 같은 조건으로 전/후 비교를 몇 번이든 다시 돌릴 수 있음

### ③ 명시적 상태 머신(FSM) — 의사코드를 보고 직접 구현
**이전 (배터리 `Enemy.cs`, 973줄)**: 상태가 bool 여러 개와 코루틴에 흩어져 있었음

```csharp
public bool isMove, activeAttack, onStun, onFlat;
protected bool die, hitted;
bool CanAttack;                                   // 지금 상태 = bool 7개의 조합

void FixedUpdate() {
    if (die || hitted || GameManager.instance.tutoInteract) return;
    if (!onStun && !CanAttack) Move();
}
IEnumerator attackActionInvoke() {                // 공격 예비동작 = 코루틴 대기
    yield return new WaitForSeconds(eStat.attackReadyTime);
    AttackAction?.Invoke(PlayerHandler.instance.CurrentPlayer.transform);
}
IEnumerator WaitDelay() {                         // 공격 후 딜레이 = 또 다른 코루틴
    yield return new WaitForSeconds(eStat.attackDelay);
    InitAttackCoolTime();                         // 여기서 CanAttack = false
}
```

**이번**: 상태를 `enum class`로 명시하고 **상태 진입(`Enter`) / 상태별 갱신(`switch`) / 타이머 전이**로 나눔 (코루틴 대신 타이머).

```cpp
void Enter(EnemyAIState& tAI, EnemyState eNext)  // 상태에 들어갈 때 한 번: 타이머를 데이터 표에서
{
    tAI.eState = eNext;
    if (eNext == EnemyState::Windup)       tAI.fTimer = kChaserDef.fWindupTime;
    else if (eNext == EnemyState::Recover) tAI.fTimer = kChaserDef.fRecoverTime;
    else                                   tAI.fTimer = 0.0f;
}

switch (tAI.eState)  // 매 틱: 지금 상태의 일만
{
case EnemyState::Chase:
    // 위치를 직접 안 바꾸고 "의도"만 기록
    tEnemy.tMoveIntent.iMoveDir = (fDx > 0.0f) ? 1 : -1;
    if (fGapX <= kChaserDef.fAttackRangeX && fDy <= kChaserDef.fAttackRangeY)
        Enter(tAI, EnemyState::Windup);
    break;
case EnemyState::Windup:   // 예비동작: 이 동안 화면에 공격 범위 표시
    if (tAI.fTimer <= 0.0f)
        Enter(tAI, EnemyState::Attack);
    break;
case EnemyState::Attack:
    tEnemy.tAttackIntent.bAttackPressed = true;
    Enter(tAI, EnemyState::Recover);
    break;
// Idle, Recover ...
}
```

- 근접 적 AI 전체 **100줄**. 같은 틀로 원거리 적·보스(3연사 / 원형탄 / 2페이즈)까지 확장 → **새 행동 = enum 값 + case 하나**
- 상태 시간은 데이터 표에서 읽고, 렌더는 상태를 읽기만 함 → 공격 범위 표시 같은 기능이 AI 코드 수정 없이 붙음

| | 이전 (배터리 `Enemy.cs`) | 이번 (`EnemyAI_U.cpp`) |
|---|---|---|
| 지금 상태 | bool 7개 조합 (`isMove`, `activeAttack`, `CanAttack`, `onStun`, `hitted`, `die`, `onFlat`) | `enum` 값 하나 |
| 예비동작·딜레이 | 코루틴 `WaitForSeconds`, 피격 시 `StopCoroutine`으로 끊어야 함 | 타이머. `Enter`에서 데이터 표 값으로 설정 |
| 이동·공격 | 적 클래스가 직접 `Move()`·물리 호출 | "의도"만 기록, 실제 이동·판정은 다른 시스템 |
| 크기 | 973줄 (AI·이동·피격·연출이 한 파일) | AI 100줄 (이동·피격·그리기는 분리) |
| 의존 | `GameManager.instance`, `PlayerHandler.instance` 싱글톤 | 인자로 받은 `World`만 |

### ④ 그 밖의 설계
- **고정 타임스텝(1/60초)**: Unity `FixedUpdate`를 직접 구현한 것. 프레임 시간을 누적했다가 1/60초씩 꺼내 로직을 돌림 → 프레임이 흔들려도 결과가 같음. 물리뿐 아니라 AI·전투 등 **게임 로직 전부**가 여기서 돌고, 그리기는 매 프레임 따로
- **책임 분리**: 매니저·싱글톤 없음. AI·입력은 "하고 싶은 것(의도)"만 기록하고, 실제 이동·피해는 각 시스템이 처리

## 4. AI 활용
- **역할 분담**: Claude = 주 개발 / Codex(GPT) = 의사코드·코드 이중 검수, C++ 학습 보조 / 개발자 = 설계 결정·핵심 구조 구현(의사코드 참고)·최종 판단
- **진행 방식**: 기능마다 의사코드 먼저 합의 (Claude 작성 → Codex 교차 검수) → 구현 → 개발자가 직접 플레이 확인 → 커밋
- **직접 구현한 부분**: 엔티티 풀(World), Spatial Hash, 근접 적 AI
  - AI와 합의한 의사코드(`docs/design/`)를 보고 C++로 직접 옮김. 문법이 막히는 곳은 Codex에게 예시를 받음. 완성 후 Claude가 리뷰, 이름 정리만 AI가 함
- **기록**: 파일 이름으로 작성자 구분(`_U` 개발자 / `_C` AI), `docs/devlog.md`에 날짜별로 "AI 제안 / 개발자가 고치거나 거절한 것과 이유"를 남김

**실제 사례**

| 상황 | AI | 개발자 |
|---|---|---|
| Spatial Hash 효과 검증 | ① 구현 전에 "FPS는 약 1.3~1.6배, 충돌 판정 로직은 약 5~8배 빨라질 것"이라는 예측을 문서에 미리 적어 둠 (측정 후 맞았는지 비교하려고)<br>② 측정 결과 정리, 칸 크기 실험 실행 | ① 의사코드를 보고 직접 구현<br>② 측정해 보니 FPS는 1.3~1.45배로 예측대로, 판정 로직은 2.3~2.6배로 **예측보다 덜 빨라짐**<br>③ 이유를 찾으려고 칸 크기 실험을 요청 → 적들이 한 자리에 수백 마리 겹쳐 있어서, 칸을 잘게 나눠도 한 칸 안의 비교 대상이 줄지 않는 것이 원인 |
| 대시 입력 | "누른 순간 대시"를 제안 (입력이 빠름) | **거절**: "짧게 누르면 대시 / 길게 누르면 달리기"가 원래 의도 → 최대 0.18초 입력 지연을 감수하고 원래 사양대로 |
| 스탯 코드 리뷰 | (Codex) 테스트로는 못 잡던 버그 2건 발견: 도달 불가 `return`, 공격 범위 기본값 0 | 직접 짠 스탯 코드에서 나온 버그 → 원인을 이해하고 반영 |
| 성능 측정 | F5 스냅샷 기록·`--bench` 자동 측정·CSV 저장 도구를 만들고 결과 표 정리 | 포폴 근거로 CSV 자료를 요청하고 첫 기준선은 직접 F5로 측정 → 이후 자동 측정으로 같은 조건 전/후 비교 |
| 피격감 | 피격 시 이동 경직은 조작감을 해친다고 비추천 | 무적 + 히트스톱 + 화면 흔들림만 선택, 빨간 테두리는 보류 |
| 발판 위 적 | 적 AI(개발자 코드)를 건드리지 않고, 이동 직전에 "발 앞이 비면 멈춤" 시스템을 끼움 | 플레이해 보고 "적이 바로 뛰어내려서 발판이 의미 없다"는 문제를 찾아 요청 |
| AI 실수 | PowerShell로 빌드 설정 파일을 고치다 한글 인코딩이 깨져 빌드 실패 → git 원본으로 복구, 재발 방지 규칙을 AI 메모리에 저장 | AI 실수도 숨기지 않고 개발 기록에 남김 |

## 5. 향후 계획
**성능**
- 적끼리 겹침 해결: 진영별로 검색 대상 나누기(충돌 레이어) + 적끼리 밀어내기 → 남은 판정 비용 줄이기
- 멀티스레드: 판정·AI를 작업 단위로 나눠 병렬 처리 후, 같은 결과가 나오는지 측정으로 확인

**네트워크 (2인 협동 → 대전)**
- 지금 구조가 이미 네트워크에 맞게 되어 있음
  - 입력이 "의도" 데이터로 분리되어 있음 → 키 입력 대신 의도만 주고받으면 됨
  - 고정 타임스텝 → 같은 입력이면 같은 결과 (틱 번호로 동기화 가능)
  - 핸들(칸 번호 + 세대) → 엔티티를 네트워크 ID로 그대로 쓸 수 있음
- 단계: 서버 권한 방식 협동 → 지연 보정(클라 예측 + 서버 보정) → 진영만 늘려서 PvP

**게임**
- 리소스 적용: 캐릭터·적 스프라이트 애니메이션, 타일셋, 효과음·배경음 파일 (지금은 사각형 + 합성음)
- 절차적 맵 생성 + 모든 방에 갈 수 있는지 검증, 비행 적 A* 경로 탐색
- 스테이지 확장(3스테이지), 아이템·적 데이터를 외부 파일로

**정리**
- README에 측정 그래프와 CSV 링크 정리

