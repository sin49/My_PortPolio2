#pragma once

struct AttackIntent;
struct MoveIntent;

// 키 입력 -> 의도 기록. 엔티티는 건드리지 않음.
// 렌더 프레임마다 호출 (고정 틱이 0번인 프레임에도 입력을 놓치지 않게).
// 키: ←→ 이동, Z 점프, X 공격, Shift 대시
class Input
{
public:
    // b~Pressed는 OR로 누적만 함. 지우는 건 고정 틱이 소비한 뒤 Game이 함.
    void Poll(MoveIntent& tMove, AttackIntent& tAttack) const;
};
