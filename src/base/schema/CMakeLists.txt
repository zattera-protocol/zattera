file(GLOB HEADERS "include/zattera/schema/*.hpp")
add_library( zattera_schema schema.cpp ${HEADERS} )
target_link_libraries( zattera_schema fc )
target_include_directories( zattera_schema PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   zattera_schema

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/zattera/schema" )
