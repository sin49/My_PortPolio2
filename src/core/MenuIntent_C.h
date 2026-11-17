#pragma once

// 메뉴 의도 (타이틀·게임 오버·승리 화면). Input이 기록, RoomFlow만 읽음.
struct MenuIntent
{
    bool bConfirmPressed = false;  // Enter. 눌린 순간 (래치 -> 첫 틱이 소비)
};
