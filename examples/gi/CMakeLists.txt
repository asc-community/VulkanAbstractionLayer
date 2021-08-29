set(SOURCES 
"EntryPoint.cpp"
)

add_executable(GI ${SOURCES})

target_link_directories(GI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(GI PUBLIC VulkanAbstractionLayer)

target_include_directories(GI PUBLIC ${VULKAN_ABSTRACTION_LAYER_INCLUDE_DIR})

target_compile_definitions(GI PUBLIC -D APPLICATION_WORKING_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}")