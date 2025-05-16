include(FetchContent)

find_package(SDL3 CONFIG REQUIRED)

FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG master
)

FetchContent_MakeAvailable(imgui)
add_library(imgui)
add_library(imgui::imgui ALIAS imgui)
target_sources(imgui 
PRIVATE
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
)
        
target_link_libraries(
    imgui
PRIVATE
    SDL3::SDL3
)

target_include_directories(imgui
PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

set_target_properties(imgui PROPERTIES FOLDER "thirdParty/imgui")

#xatlas
FetchContent_Declare(xatlas
    GIT_REPOSITORY https://github.com/jpcy/xatlas.git
    GIT_TAG master
)

FetchContent_MakeAvailable(xatlas)
add_library(xatlas)
add_library(xatlas::xatlas ALIAS xatlas)
target_sources(xatlas 
PRIVATE
    ${xatlas_SOURCE_DIR}/source/xatlas/xatlas.h
    ${xatlas_SOURCE_DIR}/source/xatlas/xatlas_c.h
    ${xatlas_SOURCE_DIR}/source/xatlas/xatlas.cpp
    $<$<CXX_COMPILER_ID:MSVC>:${xatlas_SOURCE_DIR}/source/xatlas.natvis>>
)
        
target_include_directories(xatlas
PUBLIC
    ${xatlas_SOURCE_DIR}/source/xatlas/
)

set_target_properties(xatlas PROPERTIES FOLDER "thirdParty/xatlas")

FetchContent_Declare(
  box2d
  GIT_REPOSITORY https://github.com/erincatto/box2d.git
  GIT_TAG        main
)

FetchContent_MakeAvailable(box2d)
set_target_properties(box2d PROPERTIES FOLDER "thirdParty/box2d")

FetchContent_Declare(
  vfspp
  GIT_REPOSITORY https://github.com/nextgeniuspro/vfspp.git
  GIT_TAG        v2.0.0
)

FetchContent_MakeAvailable(vfspp)
set_target_properties(vfspp PROPERTIES FOLDER "thirdParty/vfspp")

set_target_properties(miniz-cpp.test PROPERTIES FOLDER "thirdParty/miniz")
set_target_properties(example-pipe PROPERTIES FOLDER "thirdParty/miniz")
set_target_properties(example-read PROPERTIES FOLDER "thirdParty/miniz")
set_target_properties(example-write PROPERTIES FOLDER "thirdParty/miniz")

if(MSVC)
    target_compile_options(imgui PRIVATE /W0)
    target_compile_options(xatlas PRIVATE /W0)
    target_compile_options(miniz-cpp.test PRIVATE /W0)
    target_compile_options(example-pipe PRIVATE /W0)
    target_compile_options(example-read PRIVATE /W0)
    target_compile_options(example-write PRIVATE /W0)
endif()