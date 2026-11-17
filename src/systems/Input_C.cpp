#include "systems/Input_C.h"

#include "core/AttackIntent_C.h"
#include "core/MenuIntent_C.h"
#include "core/MoveIntent_C.h"

#include "raylib.h"

void Input::Poll(MoveIntent& tMove, AttackIntent& tAttack) const
{
    tMove.iMoveDir = (IsKeyDown(KEY_RIGHT) ? 1 : 0) - (IsKeyDown(KEY_LEFT) ? 1 : 0);
    tMove.bJumpHeld = IsKeyDown(KEY_Z);
    tMove.bDashHeld = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    if (IsKeyPressed(KEY_Z))
        tMove.bJumpPressed = true;
    if (IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT))
        tMove.bDashPressed = true;

    if (IsKeyPressed(KEY_X))
        tAttack.bAttackPressed = true;
}

void Input::PollMenu(MenuIntent& tMenu) const
{
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER))
        tMenu.bConfirmPressed = true;
}
