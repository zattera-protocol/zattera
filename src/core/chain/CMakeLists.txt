file(GLOB HEADERS "include/zattera/chain/*.hpp" "include/zattera/chain/utils/*.hpp")

## SORT .cpp by most likely to change / break compile
add_library( zattera_chain

             # As database takes the longest to compile, start it first
             database.cpp

             zattera_evaluator.cpp
             zattera_objects.cpp

             witness_schedule.cpp
             fork_database.cpp

             shared_authority.cpp
             block_log.cpp

             generic_custom_operation_interpreter.cpp

             utils/reward.cpp
             utils/impacted.cpp
             utils/advanced_benchmark_dumper.cpp

             ${HEADERS}
           )

target_link_libraries( zattera_chain zattera_protocol fc chainbase zattera_schema appbase
                       ${PATCH_MERGE_LIB} )
target_include_directories( zattera_chain
                            PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      zattera_chain PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

if(MSVC)
  set_source_files_properties( database.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

INSTALL( TARGETS
   zattera_chain

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/zattera/chain" )
