# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/Pl0Studio_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/Pl0Studio_autogen.dir/ParseCache.txt"
  "Pl0Studio_autogen"
  )
endif()
