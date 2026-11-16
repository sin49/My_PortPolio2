#include "core/Game_C.h"
#include "core/Tilemap_C.h"
#include "data/TestRoom_C.h"
#include "render/Render_C.h"

#include "raylib.h"

#include <optional>
#include <utility>

int main()
{
    std::optional<Tilemap> optRoom = Tilemap::FromRows(kTestRoom);
    if (!optRoom)
    {
        TraceLog(LOG_ERROR, "Test room data is invalid (row length or unknown tile char)");
        return 1;
    }

    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(kScreenWidth, kScreenHeight, "Main");

    {
        // Game은 창보다 먼저 파괴돼야 함 (나중에 텍스처 등 GPU 자원을 들게 되면 필수)
        Game tGame(std::move(*optRoom));
        tGame.Run();
    }

    CloseWindow();
    return 0;
}
