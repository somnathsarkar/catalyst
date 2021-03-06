find_package(Vulkan)
function(add_shader SHADER_FILENAME)
	set(GLSLC ${Vulkan_GLSLC_EXECUTABLE})
	set(INPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${SHADER_FILENAME}")
	set(OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER_FILENAME}.spv")
	get_filename_component(OUTPUT_DIR ${OUTPUT_PATH} DIRECTORY)
	add_custom_target(
		${SHADER_FILENAME} ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR}" COMMAND ${GLSLC} --target-env=vulkan1.1 -o ${OUTPUT_PATH} ${INPUT_PATH} -g
	)
endfunction()

function(add_shader_pair SHADER_NAME)
	add_shader("${SHADER_NAME}.vert")
	add_shader("${SHADER_NAME}.frag")
endfunction()

function(target_shaders TARGET)
	foreach(SHADER ${ARGN})
		add_dependencies(${TARGET} "${SHADER}")
	endforeach()
endfunction()

function(target_shader_pairs TARGET)
	foreach(SHADER_PAIR ${ARGN})
		add_dependencies(${TARGET} "${SHADER_PAIR}.vert" "${SHADER_PAIR}.frag")
	endforeach()
endfunction()

function(add_model MODEL_NAME)
	set(INPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/models/${MODEL_NAME}")
	set(OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}/models/${MODEL_NAME}")
	get_filename_component(OUTPUT_DIR ${OUTPUT_PATH} DIRECTORY)
	add_custom_target(
		${MODEL_NAME} ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR}" COMMAND ${CMAKE_COMMAND} -E copy ${INPUT_PATH} ${OUTPUT_PATH}
	)
endfunction()

function(target_models TARGET)
	foreach(MODEL ${ARGN})
		add_dependencies(${TARGET} ${MODEL})
	endforeach()
endfunction()

function(add_texture TEXTURE_NAME)
	set(INPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/textures/${TEXTURE_NAME}")
	set(OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}/textures/${TEXTURE_NAME}")
	get_filename_component(OUTPUT_DIR ${OUTPUT_PATH} DIRECTORY)
	add_custom_target(
		${TEXTURE_NAME} ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR}" COMMAND ${CMAKE_COMMAND} -E copy ${INPUT_PATH} ${OUTPUT_PATH}
	)
endfunction()

function(target_textures TARGET)
	foreach(TEXTURE ${ARGN})
		add_dependencies(${TARGET} ${TEXTURE})
	endforeach()
endfunction()

function(add_cubemap CUBEMAP_NAME)
	set(INPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cubemaps/${CUBEMAP_NAME}")
	set(OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}/cubemaps/${CUBEMAP_NAME}")
	get_filename_component(OUTPUT_DIR ${OUTPUT_PATH} DIRECTORY)
	string(REPLACE "/" "_" CUBEMAP_TARGET ${CUBEMAP_NAME})
	add_custom_target(
		${CUBEMAP_TARGET} ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR}" COMMAND ${CMAKE_COMMAND} -E copy_directory ${INPUT_PATH} ${OUTPUT_PATH}
	)
endfunction()

function(target_cubemaps TARGET)
	foreach(CUBEMAP ${ARGN})
		string(REPLACE "/" "_" CUBEMAP_TARGET ${CUBEMAP})
		add_dependencies(${TARGET} ${CUBEMAP_TARGET})
	endforeach()
endfunction()

function(add_billboard BILLBOARD_NAME)
	set(INPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/billboards/${BILLBOARD_NAME}")
	set(OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}/billboards/${BILLBOARD_NAME}")
	get_filename_component(OUTPUT_DIR ${OUTPUT_PATH} DIRECTORY)
	add_custom_target(
		${BILLBOARD_NAME} ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR}" COMMAND ${CMAKE_COMMAND} -E copy ${INPUT_PATH} ${OUTPUT_PATH}
	)
endfunction()

function(target_billboards TARGET)
	foreach(BILLBOARD ${ARGN})
		add_dependencies(${TARGET} ${BILLBOARD})
	endforeach()
endfunction()

add_shader_pair("phong")
add_shader_pair("pbr")
add_shader_pair("debugdraw")
add_shader_pair("depthmap")
add_shader_pair("skybox")
add_shader_pair("ssao")
add_shader_pair("hdr")
add_shader_pair("ssr")

add_shader("log_illuminance.comp")
add_shader("reduce_illuminance.comp")

add_model("bun_zipper.obj")
add_model("teapot.obj")

add_texture("black.png")
add_texture("white.png")

add_cubemap("meadow/specular")
add_cubemap("meadow/diffuse")

add_billboard("sun.png")