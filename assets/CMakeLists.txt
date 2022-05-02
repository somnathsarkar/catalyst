function(add_shader SHADER_FILENAME)
	set(GLSLC "C:/Dev/VulkanSDK/1.3.204.0/Bin/glslc.exe")
	set(INPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${SHADER_FILENAME}")
	set(OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER_FILENAME}.spv")
	get_filename_component(OUTPUT_DIR ${OUTPUT_PATH} DIRECTORY)
	add_custom_target(
		${SHADER_FILENAME} ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR}" COMMAND ${GLSLC} -o ${OUTPUT_PATH} ${INPUT_PATH}
	)
endfunction()

function(add_shader_pair SHADER_NAME)
	add_shader("${SHADER_NAME}.vert")
	add_shader("${SHADER_NAME}.frag")
endfunction()

function(target_shader_pairs TARGET)
	foreach(SHADER_PAIR ${ARGN})
		add_dependencies(${TARGET} "${SHADER_PAIR}.vert" "${SHADER_PAIR}.frag")
	endforeach()
endfunction()

add_shader_pair("phong")
add_shader_pair("pbr")
add_shader_pair("debugdraw")
add_shader_pair("depthmap")