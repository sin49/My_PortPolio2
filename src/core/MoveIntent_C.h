#pragma once

// 이동 의도. Input(플레이어) / EnemyAI(적)가 기록하고, Motor만 읽어서 속도로 바꿈.
// 기록하는 쪽은 위치·속도를 직접 건드리지 않음. 공격 의도는 AttackIntent로 분리.
struct MoveIntent
{
    int iMoveDir = 0;           // -1(왼쪽), 0, +1(오른쪽)
    bool bJumpPressed = false;  // 눌린 순간. 렌더 프레임에서 래치 -> 고정 틱이 소비 후 false
    bool bJumpHeld = false;     // 누르고 있는 중 (가변 점프)
    bool bDashPressed = false;  // 눌린 순간 = 대시 (래치, 점프와 같은 방식)
    bool bDashHeld = false;     // 누르고 있는 중 = 달리기
};
