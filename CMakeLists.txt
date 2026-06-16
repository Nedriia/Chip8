cmake_minimum_required(VERSION 3.20)

project(Chip8_Emulation
        VERSION 1.0.0
        LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Chip8_Emulation")
set(LIBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Librairies")
set(IMGUI_DIR "${PROJECT_DIR}/imgui")

set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
)

add_executable(${PROJECT_NAME}
        ${PROJECT_DIR}/main.cpp
        ${PROJECT_DIR}/Input.cpp
        ${PROJECT_DIR}/Chip8.cpp
        ${PROJECT_DIR}/Display.cpp
        ${PROJECT_DIR}/SoundManager.cpp
        ${PROJECT_DIR}/Chip8_Debugger.cpp
        ${PROJECT_DIR}/Disassembler.cpp
        ${PROJECT_DIR}/Init_RomSettings.cpp
        ${PROJECT_DIR}/Shader.cpp
        ${IMGUI_SOURCES}

        ${PROJECT_DIR}/glad.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${IMGUI_DIR}/backends
        ${IMGUI_DIR}
        ${LIBS_DIR}/Include
        ${PROJECT_DIR}/external/json
        ${PROJECT_DIR}/TinySHA1
)

target_link_directories(${PROJECT_NAME} PRIVATE
        ${LIBS_DIR}/lib
)

find_package(OpenGL REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw3
        MiniAudio
        OpenGL::GL
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
        PATH_SHADERS="${CMAKE_CURRENT_SOURCE_DIR}/assets/"
        PATH_DATABASE="${PROJECT_DIR}/chip-8-database/database/"
)