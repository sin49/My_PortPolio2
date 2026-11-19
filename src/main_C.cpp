#include "audio/SoundAsset_C.h"
#include "core/Game_C.h"
#include "render/Render_C.h"

#include "raylib.h"

#include <cstdlib>
#include <span>
#include <string>
#include <string_view>

int main(int argc, char** argv)
{
    // Main.exe --bench [--full-scan] : 자동 측정 시나리오 -> bench_results_<검색 방식>.csv 후 종료 (measure.md 6장)
    GameOptions tOptions;
    for (const char* szArg : std::span(argv, static_cast<std::size_t>(argc)).subspan(1))
    {
        const std::string_view svArg = szArg;
        if (svArg == "--bench")
            tOptions.bBenchMode = true;
        else if (svArg == "--full-scan")
            tOptions.eSpatialMode = SpatialMode::FullScan;
        else if (svArg.starts_with("--cell="))  // 해시 칸 크기 실험 (px). 잘못된 값이면 기본값 유지
        {
            const float fCell = std::strtof(std::string(svArg.substr(7)).c_str(), nullptr);
            if (fCell >= 8.0f && fCell <= 4096.0f)
                tOptions.fCellSize = fCell;
        }
    }
    const bool bBenchMode = tOptions.bBenchMode;

    // 벤치는 vsync를 꺼서 60FPS 상한 없이 실제 처리량을 잼
    if (!bBenchMode)
        SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(kScreenWidth, kScreenHeight, bBenchMode ? "Main (bench)" : "Main");
    SetExitKey(KEY_ESCAPE);
    if (bBenchMode)
        ClearWindowState(FLAG_VSYNC_HINT);  // 플래그를 안 줘도 드라이버 기본이 vsync일 수 있어서 명시적으로 끔 (swap interval 0)

    {
        // 오디오 장치: Game보다 먼저 열고 늦게 닫음 (C++은 선언 역순으로 파괴 -> tGame이 먼저 사라짐)
        AudioDeviceScope tAudioDevice;
        // Game은 창·오디오 장치보다 먼저 파괴돼야 함 (Sound를 들고 있음)
        Game tGame(tOptions);
        tGame.Run();
    }

    CloseWindow();
    return 0;
}
