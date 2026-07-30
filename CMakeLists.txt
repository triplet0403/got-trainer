cmake_minimum_required(VERSION 3.20)
project(GOTTrainer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# MinHook
FetchContent_Declare(
  minhook
  GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
  GIT_TAG master
)
FetchContent_MakeAvailable(minhook)

# ImGui
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.6
)
FetchContent_MakeAvailable(imgui)

set(IMGUI_SOURCES
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp
)

add_library(version SHARED
  src/dllmain.cpp
  src/proxy/version_proxy.cpp
  src/core/log.cpp
  src/core/pattern_scan.cpp
  src/core/game_pointers.cpp
  src/core/hook_manager.cpp
  src/core/trainer_state.cpp
  src/core/config.cpp
  src/core/hotkeys.cpp
  src/core/notify.cpp
  src/render/dx11_present_hook.cpp
  src/ui/trainer_ui.cpp
  src/features/feature_registry.cpp
  src/features/player_features.cpp
  src/features/combat_features.cpp
  src/features/movement_features.cpp
  src/features/camera_features.cpp
  src/features/item_features.cpp
  src/version.def
  ${IMGUI_SOURCES}
)

target_include_directories(version PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
  ${minhook_SOURCE_DIR}/include
)

target_link_libraries(version PRIVATE
  minhook
  d3d11
  d3d12
  dxgi
  d3dcompiler
  dwmapi
  user32
  Psapi
)

target_compile_definitions(version PRIVATE
  WIN32_LEAN_AND_MEAN
  NOMINMAX
  IMGUI_DEFINE_MATH_OPERATORS
)

set_target_properties(version PROPERTIES
  PREFIX ""
  OUTPUT_NAME "version"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
