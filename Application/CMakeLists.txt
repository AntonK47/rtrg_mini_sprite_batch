set(APPLICATION_NAME "MiniSpriteBatch")

find_package(SDL3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(gli CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Tracy CONFIG REQUIRED)

option(DOWNLOAD_ASSETS "Enables asset downoad on strurtup" ON)
option(ENABLE_TRACY "Enables Tracy profiler" OFF)

add_executable(${APPLICATION_NAME})
target_compile_features(${APPLICATION_NAME} PUBLIC cxx_std_23)
target_sources(${APPLICATION_NAME} PRIVATE 
	main.cpp
	Color.hpp
	Common.hpp
	ImGui.hpp
	ImGuiConfig.hpp
	RenderResources.hpp
	SpriteBatch.cpp
	SpriteBatch.hpp
	RenderContext.cpp
	RenderContext.hpp
	Animation.cpp
	Animation.hpp
	Game.cpp
	Game.hpp
	SampleGame.cpp
	SampleGame.hpp
	ContentManager.cpp
	ContentManager.hpp
	MapImporter.hpp
	MapImporter.cpp
	Effect.hpp
	Effect.cpp
)
target_link_libraries(
	${APPLICATION_NAME} 
PRIVATE 
	SDL3::SDL3
	imgui::imgui
	glm::glm-header-only
	glad::glad
	gli
	box2d::box2d
	vfspp::vfspp
	CURL::libcurl
	nlohmann_json::nlohmann_json
)
if(DOWNLOAD_ASSETS)
	target_compile_definitions(${APPLICATION_NAME} PUBLIC ENABLE_ASSETS_DOWNLOAD)
endif()

if(WIN32)
	target_link_options(${APPLICATION_NAME}  PRIVATE "/SUBSYSTEM:CONSOLE" "/ENTRY:mainCRTStartup")
endif()

if(MSVC)
    target_compile_options(${APPLICATION_NAME} PRIVATE /experimental:external /external:W0 /external:anglebrackets /external:Iout /external:IThirdParty)
	target_compile_options(${APPLICATION_NAME} PRIVATE /W4 /WX /MP /ZI)
	target_link_options(${APPLICATION_NAME} PRIVATE /INCREMENTAL)

else()
	target_compile_options(${APPLICATION_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(ENABLE_TRACY)
	target_link_libraries(
		${APPLICATION_NAME} 
	PRIVATE
		Tracy::TracyClient
	)
	target_compile_definitions(${APPLICATION_NAME} PUBLIC TRACY_ENABLE)
endif()

add_dependencies(${APPLICATION_NAME} CopyAssets)