#----------------------------------------------------------------
# Generated CMake target import file for configuration "MinSizeRel".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "XXCNC::xxcnc" for configuration "MinSizeRel"
set_property(TARGET XXCNC::xxcnc APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(XXCNC::xxcnc PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_MINSIZEREL "CXX"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/lib/xxcnc.lib"
  )

list(APPEND _cmake_import_check_targets XXCNC::xxcnc )
list(APPEND _cmake_import_check_files_for_XXCNC::xxcnc "${_IMPORT_PREFIX}/lib/xxcnc.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
