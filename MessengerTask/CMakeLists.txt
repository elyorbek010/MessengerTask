# CMakeList.txt : CMake project for MessengerTask, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_library (MessengerTask "src/task1_messenger.cpp")

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

FetchContent_Declare(
	CRCpp
	GIT_REPOSITORY	https://github.com/d-bahr/CRCpp.git
	GIT_TAG			release-1.2.0.0
)

FetchContent_MakeAvailable(CRCpp)

target_link_libraries(MessengerTask PRIVATE CRCpp)
target_include_directories(MessengerTask PRIVATE inc)

add_executable(messenger_tests "test/messenger_test.cpp")
target_link_libraries(messenger_tests PRIVATE Catch2::Catch2WithMain PRIVATE MessengerTask PRIVATE CRCpp)
target_include_directories(messenger_tests PRIVATE inc)
