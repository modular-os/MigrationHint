#===============================================================================
# 0. Configure utils
# Set all the shared utils that will be used by the tools.
# Notice: All the tools will be linked as STATIc libraries for easier release.
#===============================================================================

#===============================================================================
# 1. Configure the Actual binaries
# Set all the tools that will be built.
#===============================================================================

set(CA_Frontend
  CAFlexParse
)

set(CAFlexParse_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/fp_main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/jsonAbility.cpp
)

# CONFIGURE THE TOOLS
# ===================
foreach(tool ${CA_Frontend})
  # Create a library corresponding to 'plugin'
  add_executable(
    ${tool}
    ${${tool}_SOURCES}
  )

  # Configure include directories for 'tool'
  target_include_directories(
    ${tool}
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )

  # Link in the required libraries
  target_link_libraries(
    ${tool}
    "clangTooling"
  )
endforeach()