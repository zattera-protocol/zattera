list( APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/src/base/fc/GitVersionGen" )
include( GetGitRevisionDescription )
get_git_head_revision(GIT_REFSPEC ZATTERA_GIT_REVISION_SHA)
get_git_unix_timestamp(ZATTERA_GIT_REVISION_UNIX_TIMESTAMP)
git_describe(ZATTERA_GIT_REVISION_DESCRIPTION --tags)
if(NOT ZATTERA_GIT_REVISION_DESCRIPTION)
    set(ZATTERA_GIT_REVISION_DESCRIPTION "unknown")
endif(NOT ZATTERA_GIT_REVISION_DESCRIPTION)

file(GLOB HEADERS "include/zattera/utils/*.hpp")

set(sources
   benchmark_dumper.cpp
   key_conversion.cpp
   string_escape.cpp
   tempdir.cpp
   words.cpp
   logging_config.cpp
   ${HEADERS})

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/git_revision.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/git_revision.cpp" @ONLY)
list(APPEND sources "${CMAKE_CURRENT_BINARY_DIR}/git_revision.cpp")

add_library( zattera_utils
             ${sources}
             ${HEADERS} )
target_link_libraries( zattera_utils fc )
target_include_directories( zattera_utils
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
if (USE_PCH)
  set_target_properties(zattera_utils PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
  cotire(zattera_utils)
endif(USE_PCH)

if( CLANG_TIDY_EXE )
   set_target_properties(
      zattera_utils PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   zattera_utils

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/zattera/utils" )
