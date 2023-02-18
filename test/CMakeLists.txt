# GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/e4717df71a4f45bf9f0ac88c6cd9846a0bc248dd.zip)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(SNAPSHOT_TEST
  basic_test.cc
  # Add your test files here
  # foo/bar/your_test.cc
  )

add_executable(snapshot_test ${SNAPSHOT_TEST})

target_link_libraries(
  snapshot_test
  snapshot
  gtest_main
  )

include(GoogleTest)
gtest_discover_tests(snapshot_test)

