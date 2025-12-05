add_executable( zatterad main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

# Set atomic library for Linux/GCC (not needed on macOS)
if( UNIX AND NOT APPLE )
  set(atomic_library atomic )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling zatterad with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

if( ZATTERA_STATIC_BUILD )
   target_link_libraries( zatterad PRIVATE
      "-static-libstdc++ -static-libgcc"
      appbase
      zattera_utils
      zattera_plugins
      ${CMAKE_DL_LIBS}
      ${PLATFORM_SPECIFIC_LIBS}
      ${atomic_library}  # Must be last for GCC linker
   )
else( ZATTERA_STATIC_BUILD )
   target_link_libraries( zatterad PRIVATE
      appbase
      zattera_utils
      zattera_plugins
      ${CMAKE_DL_LIBS}
      ${PLATFORM_SPECIFIC_LIBS}
      ${atomic_library}  # Must be last for GCC linker
   )
endif( ZATTERA_STATIC_BUILD )

if( CLANG_TIDY_EXE )
   set_target_properties(
      zatterad PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   zatterad

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
