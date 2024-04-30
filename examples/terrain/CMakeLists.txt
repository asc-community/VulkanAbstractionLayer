set(SOURCES 
"EntryPoint.cpp"
)

add_executable(Terrain ${SOURCES})

target_link_directories(Terrain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(Terrain PUBLIC VulkanAbstractionLayer)

target_include_directories(Terrain PUBLIC ${VULKAN_ABSTRACTION_LAYER_INCLUDE_DIR})

target_compile_definitions(Terrain PUBLIC -D APPLICATION_WORKING_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}")