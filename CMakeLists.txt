cmake_minimum_required(VERSION 3.25)
project(Main LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 실행 파일(게임, 테스트)과 ASan DLL을 한 폴더에 모음: build/bin/<Config>/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ---------------------------------------------------------------------------
# AddressSanitizer(MSVC)는 런타임 검사 /RTC1과 같이 못 씀 -> Debug 기본 플래그에서 제거
# ---------------------------------------------------------------------------
option(GAME_ASAN "Debug 빌드에서 AddressSanitizer 사용" ON)

if(MSVC)
    if(GAME_ASAN)
        foreach(flagVar CMAKE_C_FLAGS_DEBUG CMAKE_CXX_FLAGS_DEBUG)
            string(REGEX REPLACE "/RTC[1csu]*" "" ${flagVar} "${${flagVar}}")
        endforeach()
    endif()
    # 소스를 UTF-8로 읽음 (한글 주석 + 외부 라이브러리의 비 ASCII 문자 -> C4819 방지).
    # 외부 라이브러리에도 적용되도록 FetchContent 전에 전역으로 설정.
    add_compile_options(/utf-8)

    # ASan 라이브러리가 설치돼 있는지 미리 확인 (없으면 링크 단계에서 LNK1104로 늦게 터짐)
    if(GAME_ASAN)
        get_filename_component(msvcToolRoot "${CMAKE_CXX_COMPILER}/../../../.." ABSOLUTE)
        if(NOT EXISTS "${msvcToolRoot}/lib/x64/clang_rt.asan_dynamic_runtime_thunk-x86_64.lib")
            message(FATAL_ERROR
                "MSVC AddressSanitizer 라이브러리가 없음: ${msvcToolRoot}/lib/x64\n"
                "Visual Studio Installer > 수정 > 개별 구성 요소 > 'Windows용 C++ AddressSanitizer' 설치 후 다시 구성하거나,\n"
                "임시로 -DGAME_ASAN=OFF 로 구성할 것.")
        endif()
    endif()
endif()

# ---------------------------------------------------------------------------
# 외부 라이브러리 (FetchContent)
# SYSTEM: 외부 헤더 경고는 우리 /W4 대상에서 제외
# ---------------------------------------------------------------------------
include(FetchContent)
# 한 번 받은 뒤에는 재구성할 때 원격 저장소 확인(git fetch)을 건너뜀 (태그 고정이라 바뀔 일 없음)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "" FORCE)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)   # raylib 예제 빌드 안 함
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE
    SYSTEM
)

set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.11
    GIT_SHALLOW    TRUE
    SYSTEM
)

FetchContent_MakeAvailable(raylib doctest)

# ---------------------------------------------------------------------------
# 우리 코드 공통 옵션: /W4, UTF-8 소스, Debug에서 ASan
# ---------------------------------------------------------------------------
function(game_set_options target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
        if(GAME_ASAN)
            target_compile_options(${target} PRIVATE $<$<CONFIG:Debug>:/fsanitize=address>)
            # ASan은 증분 링크와 호환 안 됨
            target_link_options(${target} PRIVATE $<$<CONFIG:Debug>:/INCREMENTAL:NO>)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        if(GAME_ASAN)
            target_compile_options(${target} PRIVATE $<$<CONFIG:Debug>:-fsanitize=address -fno-omit-frame-pointer>)
            target_link_options(${target} PRIVATE $<$<CONFIG:Debug>:-fsanitize=address>)
        endif()
    endif()
    target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/src)
endfunction()

# MSVC ASan 런타임 DLL을 실행 파일 옆에 복사 (VS 밖에서 exe/ctest 실행 가능하게)
function(game_copy_asan_runtime target)
    if(MSVC AND GAME_ASAN)
        get_filename_component(msvcBinDir "${CMAKE_CXX_COMPILER}" DIRECTORY)
        foreach(dll clang_rt.asan_dynamic-x86_64.dll clang_rt.asan_dbg_dynamic-x86_64.dll)
            if(EXISTS "${msvcBinDir}/${dll}")
                add_custom_command(TARGET ${target} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            "${msvcBinDir}/${dll}" "$<TARGET_FILE_DIR:${target}>"
                )
            endif()
        endforeach()
    endif()
endfunction()

# ---------------------------------------------------------------------------
# 게임 로직 라이브러리: raylib에 링크하지 않음 (= raylib 헤더를 include하면 빌드가 깨짐).
# 게임과 테스트가 같이 씀 -> "렌더 없이 로직 테스트 가능"을 빌드 구조로 강제.
# ---------------------------------------------------------------------------
# 파일명 규칙: AI 작성 = 이름_C / 사용자 작성 = 이름_U / 섞인 파일 = 접미사 없음 (CLAUDE.md 명명)
add_library(GameLogic STATIC
    src/core/Tilemap_C.cpp
    src/core/World_U.cpp
    src/systems/PlayerMotor_C.cpp
    src/systems/Physics_C.cpp
    src/systems/Combat_C.cpp
    src/systems/SpatialQuery_C.cpp
)
target_include_directories(GameLogic PUBLIC ${CMAKE_SOURCE_DIR}/src)
game_set_options(GameLogic)

# ---------------------------------------------------------------------------
# 게임 실행 파일 (raylib 의존 부분: 창, 입력, 렌더, 메인 루프)
# ---------------------------------------------------------------------------
add_executable(Main
    src/main_C.cpp
    src/core/Game_C.cpp
    src/systems/Input_C.cpp
    src/render/Render_C.cpp
    src/render/Particles_C.cpp
)
target_link_libraries(Main PRIVATE GameLogic raylib)
game_set_options(Main)
game_copy_asan_runtime(Main)
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Main)

# ---------------------------------------------------------------------------
# 테스트 (raylib 비의존 로직만)
# ---------------------------------------------------------------------------
enable_testing()

add_executable(tests
    tests/TestMain_C.cpp
    tests/FixedTimestepTest_C.cpp
    tests/TilemapTest_C.cpp
    tests/PhysicsTest_C.cpp
    tests/PlayerMotorTest_C.cpp
    tests/CombatTest_C.cpp
    tests/HitTest_C.cpp
    tests/WorldTest_C.cpp       # World 스펙 테스트 (AI 작성, 사용자 구현이 통과)
)
target_link_libraries(tests PRIVATE GameLogic doctest::doctest)
game_set_options(tests)
game_copy_asan_runtime(tests)

add_test(NAME tests COMMAND tests)
