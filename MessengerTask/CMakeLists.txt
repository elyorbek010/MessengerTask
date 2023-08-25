# CMakeList.txt : CMake project for MessengerTask, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (MessengerTask "task1_messenger.cpp" "task1_messenger.hpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET MessengerTask PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.0.1 # or a later release
)

FetchContent_MakeAvailable(Catch2)

add_executable(tests test.cpp)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)