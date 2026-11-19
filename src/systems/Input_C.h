#pragma once

struct AttackIntent;
struct DebugIntent;
struct MenuIntent;
struct MoveIntent;

// 키 입력 -> 의도 기록. 엔티티는 건드리지 않음.
// 렌더 프레임마다 호출 (고정 틱이 0번인 프레임에도 입력을 놓치지 않게).
// 키: ←→ 이동, Z 점프, X 공격, Shift 대시(누른 순간)/달리기(누르고 있기), Enter 확인
class Input
{
public:
    // b~Pressed는 OR로 누적만 함. 지우는 건 고정 틱이 소비한 뒤 Game이 함.
    void Poll(MoveIntent& tMove, AttackIntent& tAttack) const;
    void PollMenu(MenuIntent& tMenu) const;
    void PollDebug(DebugIntent& tDebug) const;  // F1 디버그(패널+무적), F2 소환 1 (꾹 = 키 반복), F3 검색 방식, F4 소환 100, F5 측정
};
