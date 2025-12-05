file(GLOB HEADERS "include/zattera/chain_plugin/*.hpp")
add_library( chain_plugin
             chain_plugin.cpp
             ${HEADERS} )

# Set atomic library for Linux/GCC (not needed on macOS)
if( UNIX AND NOT APPLE )
  set(atomic_library atomic )
endif()

target_link_libraries( chain_plugin zattera_chain appbase zattera_utils statsd_plugin ${atomic_library} )
target_include_directories( chain_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      chain_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   chain_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/zattera/chain_plugin" )
