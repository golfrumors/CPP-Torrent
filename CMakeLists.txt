cmake_minimum_required(VERSION 3.19)

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
FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/whoshuu/cpr.git 
                         GIT_TAG 0817715923c9705e68994eb52ef9df3f6845beba) #the commit hash for 1.10.0
FetchContent_MakeAvailable(cpr)

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
                             src/TorrentFileParser.cpp 
                             src/PeerRetriever.cpp 
                             src/utils.cpp 
                             src/PeerConnection.cpp 
                             src/connect.cpp   
                             src/TorrentClient.cpp 
                             src/BitTorrentMessage.cpp 
                             src/PieceManager.cpp 
                             src/Piece.cpp
							 src/Sha.cpp
							 include/Sha.h
                             include/TorrentFileParser.h 
                             include/PeerRetriever.h
                             include/utils.h
                             include/PeerConnection.h
                             include/connect.h
                             include/TorrentClient.h
                             include/BitTorrentMessage.h
                             include/PieceManager.h
                             include/Piece.h
                             include/Block.h 
                             include/SharedQueue.h)


target_link_libraries(TorrentClient PRIVATE bencoding cpr loguru ${CURL_LIBRARIES} ${OPENSSL_LIBRARIES})
