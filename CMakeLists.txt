cmake_minimum_required(VERSION 3.19)

set(CMAKE_CXX_fLAGS "${CMAKE_CXX_FLAGS} -mavx")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_USE_NANOSLEEP")


set(CMAKE_CXX_COMPILER "g++")

project(TorrentClient VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
include_directories(lib)
include_directories(include)

#Bencoding
file(GLOB BENCODING_SRC "lib/bencode/*.cpp" "include/bencode/*.h")
add_library(bencoding STATIC ${BENCODING_SRC})

#Loguru
add_library(loguru STATIC lib/loguru/loguru.cpp lib/loguru/loguru.hpp)

#cpr
include(FetchContent)
FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
                         GIT_TAG 8f6c994a6044762adbce8ea22e0cf740b8ce79d5) # The commit hash for 1.8.1. Replace with the latest from: https://github.com/libcpr/cpr/releases
FetchContent_MakeAvailable(cpr) #the commit hash for 1.10.0

#curl
find_package(CURL)
include_directories(${CURL_INCLUDE_DIRS})

#pkgconfig & openssl
find_package(PkgConfig REQUIRED)
pkg_search_module(OPENSSL REQUIRED openssl)

if( OPENSSL_FOUND )
    include_directories(${OPENSSL_INCLUDE_DIRS})
    message(STATUS "Using OpenSSL ${OPENSSL_VERSION}")
else()
    # Error; with REQUIRED, pkg_search_module() will throw an error by it's own
endif()

add_executable(TorrentClient src/main.cpp 
                             src/TorrentParser.cpp 
                             src/PeerRet.cpp 
                             src/Utils.cpp 
                             src/PeerConn.cpp 
							 src/Connect.cpp   
                             src/TorrentClient.cpp 
                             src/BitMessage.cpp 
                             src/PieceMan.cpp 
                             src/Piece.cpp
							 src/Sha.cpp
							 include/Sha.h
                             include/TorrentParser.h 
                             include/PeerRet.h
                             include/Utils.h
                             include/PeerConn.h
							 include/Connect.h
                             include/TorrentClient.h
                             include/BitMessage.h
                             include/PieceMan.h
                             include/Piece.h
                             include/Block.h 
                             include/SharedQueue.h)


target_link_libraries(TorrentClient PRIVATE bencoding cpr::cpr loguru ${CURL_LIBRARIES} ${OPENSSL_LIBRARIES})
