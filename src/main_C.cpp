#include "core/Game_C.h"
#include "render/Render_C.h"

#include "raylib.h"

int main()
{
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(kScreenWidth, kScreenHeight, "Main");
    SetExitKey(KEY_ESCAPE);

    {
        // Game은 창보다 먼저 파괴돼야 함 (나중에 텍스처·사운드 등 GPU/오디오 자원을 들게 되면 필수)
        Game tGame;
        tGame.Run();
    }

    CloseWindow();
    return 0;
}
