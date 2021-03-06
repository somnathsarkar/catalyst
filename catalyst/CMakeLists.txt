﻿add_library(catalyst)

find_package(Vulkan)
cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH PARENT_DIR)
target_include_directories(catalyst PUBLIC ${PARENT_DIR})
target_include_directories(catalyst PUBLIC "${Vulkan_INCLUDE_DIRS}" "../external/glm" "../external/glfw-3.3.7/include" "../external/assimp/include")
target_link_libraries(catalyst PUBLIC "${Vulkan_LIBRARIES}" "glfw" ${GLFW_LIBRARIES} "assimp")
target_compile_features(catalyst PUBLIC cxx_std_17)

target_sources(catalyst PRIVATE 
"dev/dev.h"
"script/script.h"
"script/script.cc"
"render/renderer.h"
"render/renderer.cc"
"render/renderer_device.cc"
"render/renderer_scene.cc"
"render/renderer_surface.cc"
"render/renderer_debugdraw.cc"
"render/renderer_descriptor.cc"
"render/renderer_depthmap.cc"
"render/renderer_shadowmap.cc"
"render/renderer_utilities.cc"
"render/renderer_skybox.cc"
"render/renderer_ssao.cc"
"render/renderer_hdr.cc"
"render/renderer_illuminance.cc"
"render/renderer_ssr.cc"
"application/application.h"
"application/application.cc"
"window/window.h"
"window/window.cc"
"window/glfw/glfwwindow.h"
"window/glfw/glfwwindow.cc"
"time/timemanager.cc"
"time/timemanager.h"
"scene/sceneobject.h"
"scene/sceneobject.cc"
"scene/scene.h"
"scene/scene.cc"
"input/inputmanager.h"
"input/inputmanager.cc"
"scene/debugdrawobject.h"
"scene/debugdrawobject.cc"
"scene/transform.h"
"scene/transform.cc"
"scene/propertymanager.h"
"scene/propertymanager.cc"
"scene/resource.h"
"scene/resource.cc"
"filesystem/importer.h"
"filesystem/importer.cc")

target_shader_pairs(catalyst "phong" "debugdraw" "depthmap" "pbr" "skybox" "ssao" "hdr" "ssr")
target_shaders(catalyst "log_illuminance.comp" "reduce_illuminance.comp")
target_models(catalyst "bun_zipper.obj" "teapot.obj")
target_textures(catalyst "black.png" "white.png")
target_cubemaps(catalyst "meadow/specular" "meadow/diffuse")
target_billboards(catalyst "sun.png")

if(MSVC)
  target_compile_options(catalyst PUBLIC "/Zi")
  target_link_options(catalyst PUBLIC "/INCREMENTAL")
endif()