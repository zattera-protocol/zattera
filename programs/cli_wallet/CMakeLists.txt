add_executable( cli_wallet main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling cli_wallet with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

if( ZATTERA_STATIC_BUILD )
   target_link_libraries( cli_wallet PRIVATE "-static-libstdc++ -static-libgcc -lreadline" )
endif( ZATTERA_STATIC_BUILD )

target_link_libraries( cli_wallet PRIVATE
                       graphene_net zattera_chain zattera_protocol zattera_utils zattera_wallet
                       account_by_key_api_plugin account_history_api_plugin database_api_plugin block_api_plugin
                       follow_api_plugin reputation_api_plugin market_history_api_plugin network_broadcast_api_plugin
                       tags_api_plugin witness_api_plugin
                       fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

if( CLANG_TIDY_EXE )
   set_target_properties(
      cli_wallet PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

if(MSVC)
  set_source_files_properties( main.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

install( TARGETS
   cli_wallet

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
