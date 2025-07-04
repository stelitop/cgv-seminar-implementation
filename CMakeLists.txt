cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project(OrigamiSimulator C CXX)

# Set this before including framework such that it knows to use the OpenGL4.5 version of GLAD
if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/external_code")
	# Create framework library and include CMake scripts (compiler warnings, sanitizers and static analyzers).
	add_subdirectory("external_code")
elseif(PROJECT_IS_TOP_LEVEL)
	add_subdirectory("../../../external_code/" "${CMAKE_CURRENT_BINARY_DIR}/external_code/")
endif()
	
add_executable(OrigamiSimulatorImplementation
    "src/application.cpp"
    "src/texture.cpp"
	"src/mesh.cpp"
	"src/camera.cpp"
	"src/origami.cpp"
	"src/origamiexception.h" 
	"src/glyph_drawer.cpp" 
	"src/settings.cpp")	

target_compile_definitions(OrigamiSimulatorImplementation PRIVATE RESOURCE_ROOT="${CMAKE_CURRENT_LIST_DIR}/")
target_compile_features(OrigamiSimulatorImplementation PRIVATE cxx_std_20)
target_link_libraries(OrigamiSimulatorImplementation PRIVATE CGFramework)
enable_sanitizers(OrigamiSimulatorImplementation)
set_project_warnings(OrigamiSimulatorImplementation)

# Copy all files in the resources folder to the build directory after every successful build.
add_custom_command(TARGET OrigamiSimulatorImplementation POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${CMAKE_CURRENT_LIST_DIR}/resources/" "$<TARGET_FILE_DIR:OrigamiSimulatorImplementation>/resources/")

add_custom_command(TARGET OrigamiSimulatorImplementation POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${CMAKE_CURRENT_LIST_DIR}/origami_examples/" "$<TARGET_FILE_DIR:OrigamiSimulatorImplementation>/origami_examples/")

# We would like to copy the files when they changed. Even if no *.cpp files were modified (and
# thus no build is triggered). We tell CMake that the executable depends on the shader files in
# the build directory. We also tell it how to generate those files (by copying them from the
# shaders folder in this directory). The gather all glsl files in the shaders folder when CMake
# is configured. So if you were to add a shader file then you need to configure CMake again.
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/shaders/")

set(OrigamiSimulatorImplementation_shader_copies "")
# List all *.glsl files in the shaders folder
file(GLOB OrigamiSimulatorImplementation_shader_sources "${CMAKE_CURRENT_LIST_DIR}/shaders/*.glsl")
foreach (shader_file IN LISTS OrigamiSimulatorImplementation_shader_sources)
	get_filename_component(file_name ${shader_file} NAME)
	message("shader_file: ${file_name}")
	add_custom_command(
		OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/shaders/${file_name}"
		COMMAND ${CMAKE_COMMAND} -E copy
		"${CMAKE_CURRENT_LIST_DIR}/shaders/${file_name}"
		"${CMAKE_CURRENT_BINARY_DIR}/shaders/${file_name}"
		DEPENDS "${CMAKE_CURRENT_LIST_DIR}/shaders/${file_name}"
		)
	LIST(APPEND OrigamiSimulatorImplementation_shader_copies "${CMAKE_CURRENT_BINARY_DIR}/shaders/${file_name}")
endforeach()
add_custom_target(OrigamiSimulatorImplementation_copy_shaders DEPENDS ${OrigamiSimulatorImplementation_shader_copies})
add_dependencies(OrigamiSimulatorImplementation OrigamiSimulatorImplementation_copy_shaders)

