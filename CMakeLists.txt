cmake_minimum_required(VERSION 3.10)
project(live_translator_stream C CXX)

set(CMAKE_CXX_STANDARD 11)

# Add whisper.cpp as a subdirectory
add_subdirectory(whisper.cpp)

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
include_directories(${Python3_INCLUDE_DIRS})

# Add your main.cpp executable and all required example sources
add_executable(main
    main.cpp
    whisper.cpp/examples/common.cpp
    whisper.cpp/examples/common-sdl.cpp
    whisper.cpp/examples/common-whisper.cpp
)

# Include directories for headers
target_include_directories(main PRIVATE
    whisper.cpp
    whisper.cpp/include
    whisper.cpp/examples
)

# Link against whisper, Python, SDL2, pthreads
target_link_libraries(main PRIVATE whisper ${Python3_LIBRARIES})

find_package(SDL2 REQUIRED)
target_include_directories(main PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(main PRIVATE ${SDL2_LIBRARIES})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(main PRIVATE Threads::Threads)