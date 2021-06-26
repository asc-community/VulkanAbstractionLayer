# Install script for directory: /home/artamonovoleg/Documents/VulkanAbstractionLayer/submodules/glslang/SPIRV

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/artamonovoleg/Documents/VulkanAbstractionLayer/build/submodules/glslang/SPIRV/libSPVRemapper.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/artamonovoleg/Documents/VulkanAbstractionLayer/build/submodules/glslang/SPIRV/libSPIRV.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SPVRemapperTargets.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SPVRemapperTargets.cmake"
         "/home/artamonovoleg/Documents/VulkanAbstractionLayer/build/submodules/glslang/SPIRV/CMakeFiles/Export/lib/cmake/SPVRemapperTargets.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SPVRemapperTargets-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SPVRemapperTargets.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake" TYPE FILE FILES "/home/artamonovoleg/Documents/VulkanAbstractionLayer/build/submodules/glslang/SPIRV/CMakeFiles/Export/lib/cmake/SPVRemapperTargets.cmake")
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake" TYPE FILE FILES "/home/artamonovoleg/Documents/VulkanAbstractionLayer/build/submodules/glslang/SPIRV/CMakeFiles/Export/lib/cmake/SPVRemapperTargets-debug.cmake")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SPIRVTargets.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SPIRVTargets.cmake"
         "/home/artamonovoleg/Documents/VulkanAbstractionLayer/build/submodules/glslang/SPIRV/CMakeFiles/Export/lib/cmake/SPIRVTargets.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SPIRVTargets-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SPIRVTargets.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake" TYPE FILE FILES "/home/artamonovoleg/Documents/VulkanAbstractionLayer/build/submodules/glslang/SPIRV/CMakeFiles/Export/lib/cmake/SPIRVTargets.cmake")
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake" TYPE FILE FILES "/home/artamonovoleg/Documents/VulkanAbstractionLayer/build/submodules/glslang/SPIRV/CMakeFiles/Export/lib/cmake/SPIRVTargets-debug.cmake")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/glslang/SPIRV" TYPE FILE FILES
    "/home/artamonovoleg/Documents/VulkanAbstractionLayer/submodules/glslang/SPIRV/bitutils.h"
    "/home/artamonovoleg/Documents/VulkanAbstractionLayer/submodules/glslang/SPIRV/spirv.hpp"
    "/home/artamonovoleg/Documents/VulkanAbstractionLayer/submodules/glslang/SPIRV/GLSL.std.450.h"
    "/home/artamonovoleg/Documents/VulkanAbstractionLayer/submodules/glslang/SPIRV/GLSL.ext.EXT.h"
    "/home/artamonovoleg/Documents/VulkanAbstractionLayer/submodules/glslang/SPIRV/GLSL.ext.KHR.h"
    "/home/artamonovoleg/Documents/VulkanAbstractionLayer/submodules/glslang/SPIRV/GlslangToSpv.h"
    "/home/artamonovoleg/Documents/VulkanAbstractionLayer/submodules/glslang/SPIRV/hex_float.h"
    "/home/artamonovoleg/Documents/VulkanAbstractionLayer/submodules/glslang/SPIRV/Logger.h"
    "/home/artamonovoleg/Documents/VulkanAbstractionLayer/submodules/glslang/SPIRV/SpvBuilder.h"
    "/home/artamonovoleg/Documents/VulkanAbstractionLayer/submodules/glslang/SPIRV/spvIR.h"
    "/home/artamonovoleg/Documents/VulkanAbstractionLayer/submodules/glslang/SPIRV/doc.h"
    "/home/artamonovoleg/Documents/VulkanAbstractionLayer/submodules/glslang/SPIRV/SpvTools.h"
    "/home/artamonovoleg/Documents/VulkanAbstractionLayer/submodules/glslang/SPIRV/disassemble.h"
    "/home/artamonovoleg/Documents/VulkanAbstractionLayer/submodules/glslang/SPIRV/GLSL.ext.AMD.h"
    "/home/artamonovoleg/Documents/VulkanAbstractionLayer/submodules/glslang/SPIRV/GLSL.ext.NV.h"
    "/home/artamonovoleg/Documents/VulkanAbstractionLayer/submodules/glslang/SPIRV/NonSemanticDebugPrintf.h"
    "/home/artamonovoleg/Documents/VulkanAbstractionLayer/submodules/glslang/SPIRV/SPVRemapper.h"
    "/home/artamonovoleg/Documents/VulkanAbstractionLayer/submodules/glslang/SPIRV/doc.h"
    )
endif()

