set(SOURCES 
"EntryPoint.cpp"
)

add_executable(LTC ${SOURCES})

target_link_directories(LTC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(LTC PUBLIC VulkanAbstractionLayer)

target_include_directories(LTC PUBLIC ${VULKAN_ABSTRACTION_LAYER_INCLUDE_DIR})

target_compile_definitions(LTC PUBLIC -D APPLICATION_WORKING_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}")