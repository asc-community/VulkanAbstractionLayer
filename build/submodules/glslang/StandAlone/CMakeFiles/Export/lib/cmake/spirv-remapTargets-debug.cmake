#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "spirv-remap" for configuration "Debug"
set_property(TARGET spirv-remap APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(spirv-remap PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/spirv-remap"
  )

list(APPEND _IMPORT_CHECK_TARGETS spirv-remap )
list(APPEND _IMPORT_CHECK_FILES_FOR_spirv-remap "${_IMPORT_PREFIX}/bin/spirv-remap" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
