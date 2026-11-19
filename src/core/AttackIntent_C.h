#pragma once

#include "core/Vec2_C.h"

// 공격 의도. Input / EnemyAI / ShooterAI가 기록하고, Combat·Projectiles만 읽음.
struct AttackIntent
{
    bool bAttackPressed = false;  // 근접: 눌린 순간. 래치 -> 고정 틱이 소비 후 false

    // 원거리 (Shooter·보스): 이번 틱 발사 요청. 실제 생성은 Projectiles (AI는 의도만)
    bool bFirePressed = false;
    Vec2 vAim = {};            // 가운데 탄의 방향 (길이 무관, 0이면 안 쏨)
    int iShotCount = 1;        // 탄 수
    float fSpreadDeg = 0.0f;   // 부채꼴 전체 각도. 360 이상 = 원형으로 고르게
};
