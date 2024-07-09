# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)


idf_component_register(SRCS ${app_sources})

target_add_binary_data(${COMPONENT_TARGET} "html/index.html" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "html/config.html" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "html/charts.js" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "html/zoom.min.js" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "html/hammer.min.js" TEXT)