cmake_minimum_required(VERSION 3.22)

# Windows(MSVC): VCランタイムも静的にして VCRUNTIME140.dll 不要にする
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(JuicedEditor CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# macOS: Intel / Apple Silicon 両対応(不要なら削除可)
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif()

# SFMLを静的ライブラリとして取得・ビルド
include(FetchContent)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)

# CMake 4.x で、依存ライブラリ(freetype等)の古い cmake_minimum_required を許可する
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.0
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(SFML)

# WIN32 を付けるとコンソールなしのGUIアプリになる(Windows以外では無視される)
add_executable(juiced_editor WIN32 main.cpp)
target_link_libraries(juiced_editor PRIVATE SFML::Graphics SFML::Window SFML::System)

# Windows: GUIアプリでも main() をそのまま使えるようにする
if(WIN32)
    target_link_libraries(juiced_editor PRIVATE SFML::Main)
endif()

# assets/ を実行ファイルの隣にコピー(ローカル実行用)
add_custom_command(TARGET juiced_editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:juiced_editor>/assets
)
