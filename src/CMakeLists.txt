#===============================================================================
# 0. Configure utils
# Set all the shared utils that will be used by the tools.
# Notice: All the tools will be linked as STATIC libraries for easier release.
#===============================================================================

# Add ca_utils.cpp as a library
add_library(ca_utils STATIC ca_utils.cpp)

# Configure include directories for ca_utils library
target_include_directories(ca_utils PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/../include"
)

#===============================================================================
# 1. Configure the Actual binaries
# Set all the tools that will be built.
#===============================================================================

set(CodeAnalysis_TOOLS
  CodeAnalysis
)

set(CodeAnalysis_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/CodeAnalysis.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/ca_PreprocessorHelpers.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/ca_ASTHelpers.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/ca_MigrateCodeGenerator.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/ca_ExDepJSONBackend.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/ca_Abilities.cpp"
)

# CONFIGURE THE TOOLS
# ===================
foreach(tool ${CodeAnalysis_TOOLS})
  # Create a library corresponding to 'plugin'
  add_executable(
    ${tool}
    ${${tool}_SOURCES}
  )

  # Configure include directories for 'tool'
  target_include_directories(
    ${tool}
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../include"
  )

  # Link in the required libraries
  target_link_libraries(
    ${tool}
    "clangTooling"
    ca_utils
  )
endforeach()

add_subdirectory(FlexiParse)