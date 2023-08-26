set(CodeAnalysis_TOOLS
    CodeAnalysis
)

set(CodeAnalysis_SOURCES
 "${CMAKE_CURRENT_SOURCE_DIR}/CodeAnalysis.cpp"
)

# CONFIGURE THE TOOLS
# ===================
foreach( tool ${CodeAnalysis_TOOLS} )
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
    )
endforeach()