set(APPLICATION_NAME "MiniSpriteBatch")

find_package(SDL3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(gli CONFIG REQUIRED)

add_executable(${APPLICATION_NAME})
target_compile_features(${APPLICATION_NAME} PUBLIC cxx_std_23)
target_sources(${APPLICATION_NAME} PRIVATE 
	main.cpp SpriteBatch.cpp RenderContext.cpp Animation.cpp Game.cpp SampleGame.cpp ContentManager.cpp)
target_link_libraries(
	${APPLICATION_NAME} 
PRIVATE 
	SDL3::SDL3
	imgui::imgui
	glm::glm-header-only
	glad::glad
	tiny_dds::tiny_dds
	gli
)

if(WIN32)
	target_link_options(${APPLICATION_NAME}  PRIVATE "/SUBSYSTEM:CONSOLE" "/ENTRY:mainCRTStartup")
endif()


add_dependencies(${APPLICATION_NAME} CopyAssets)