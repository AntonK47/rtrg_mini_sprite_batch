cmake_minimum_required(VERSION 3.30)

project(rtrg VERSION 0.1 LANGUAGES CXX)

set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Assets")
	foreach(RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Assets")
		list(APPEND commands
			COMMAND ${CMAKE_COMMAND} -E copy_directory 
                ${RESOURCE_DIR}
                "${CMAKE_CURRENT_BINARY_DIR}/Application/Assets")
	endforeach()
endif()

add_custom_target(CopyAssets ALL COMMAND ${commands})
set_property(TARGET CopyAssets PROPERTY FOLDER "utility")

add_subdirectory(Application)
add_subdirectory(ThirdParty EXCLUDE_FROM_ALL TRUE)