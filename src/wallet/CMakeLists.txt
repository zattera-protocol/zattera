file(GLOB HEADERS "include/zattera/wallet/*.hpp")

find_package( Perl )
find_package( Doxygen )

# Disable Perl-based wallet documentation generation to avoid DoxyDocs.pm dependency
# The standin version provides runtime documentation via reflection instead
# Original condition: if( PERL_FOUND AND DOXYGEN_FOUND AND NOT "${CMAKE_GENERATOR}" STREQUAL "Ninja" )
if( FALSE )
  configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile )
  add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/doxygen/perlmod/DoxyDocs.pm
                      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                      COMMAND ${DOXYGEN_EXECUTABLE}
                      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile ${CMAKE_CURRENT_SOURCE_DIR}/include/zattera/wallet/wallet.hpp )

  add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp
                      COMMAND PERLLIB=${CMAKE_CURRENT_SOURCE_DIR} ${PERL_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_api_documentation.pl ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp.new

                      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp.new ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp
                      COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp.new
                      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/generate_api_documentation.pl ${CMAKE_CURRENT_BINARY_DIR}/doxygen/perlmod/DoxyDocs.pm )
else()
  # no perl or doxygen, generate the best docs we can at runtime from reflection
  add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp
                      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/api_documentation_standin.cpp ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp
                      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/api_documentation_standin.cpp )
endif()

add_library( zattera_wallet wallet.cpp remote_node_api.cpp ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp ${HEADERS} )
target_link_libraries( zattera_wallet PRIVATE
   graphene_net
   zattera_chain
   zattera_protocol
   zattera_utils
   fc
   account_by_key_api_plugin
   account_history_api_plugin
   database_api_plugin
   block_api_plugin
   follow_api_plugin
   reputation_api_plugin
   market_history_api_plugin
   network_broadcast_api_plugin
   tags_api_plugin
   witness_api_plugin
   ${CMAKE_DL_LIBS}
   ${PLATFORM_SPECIFIC_LIBS} )
target_include_directories( zattera_wallet PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      zattera_wallet PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

if(MSVC)
  set_source_files_properties( wallet.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

install( TARGETS
   zattera_wallet

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/zattera/wallet" )
