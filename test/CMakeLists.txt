add_executable(
    unit_tests
    memcache_lru.cpp
    memcache_cmds.cpp
    )

target_link_libraries(
    unit_tests
    gtest_main
    memcache
    )

add_test(
  NAME
    unit
  COMMAND
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/unit_tests
  )
