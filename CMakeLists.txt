cmake_minimum_required(VERSION 2.8)

set(PROJ_NAME FlvPlayer)

project(${PROJ_NAME})

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} CUR_DIR_SRCS)

file(GLOB_RECURSE CUR_DIR_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

source_group("Header Files"
	FILES ${CUR_DIR_HEADERS})

# set(LOCAL_LIB_DIR "C:\\workspace\\local\\lib")
# set(LOCAL_INC_DIR "C:\\workspace\\local\\include")

# set(FFMPEG_INC_DIR "C:\\workspace\\c++\\ffmpeg\\ffmpeg-4.1.1-win64-dev\\include")
# set(FFMPEG_LIB_DIR "C:\\workspace\\c++\\ffmpeg\\ffmpeg-4.1.1-win64-dev\\lib")

set(FFMPEG_ROOT_DIR C:\\workspace\\c++\\ffmpeg\\FFmpeg-n4.1.1\\output)
set(FFMPEG_INC_DIR ${FFMPEG_ROOT_DIR}/include)
set(FFMPEG_LIB_DIR ${FFMPEG_ROOT_DIR}/bin)

message(${FFMPEG_INC_DIR})
message(${FFMPEG_LIB_DIR})

set(SDL_ROOT_DIR C:\\workspace\\c++\\SDL\\SDL2-devel-2.0.9-VC\\SDL2-2.0.9)
set(SDL_INC_DIR ${SDL_ROOT_DIR}/include)
set(SDL_LIB_DIR ${SDL_ROOT_DIR}/lib/x64)

include_directories(${FFMPEG_INC_DIR} ${SDL_INC_DIR})

find_library(LIB_AVCODEC
    avcodec
    PATHS ${FFMPEG_LIB_DIR}
    NO_DEFAULT_PATH)

find_library(LIB_AVFORMAT
    avformat
    PATHS ${FFMPEG_LIB_DIR}
    NO_DEFAULT_PATH)

find_library(LIB_AVUTIL
    avutil
    PATHS ${FFMPEG_LIB_DIR}
    NO_DEFAULT_PATH)
	
find_library(LIB_SWSCALE
    swscale
    PATHS ${FFMPEG_LIB_DIR}
    NO_DEFAULT_PATH)
	
find_library(LIB_SWRESAMPLE
    swresample
    PATHS ${FFMPEG_LIB_DIR}
    NO_DEFAULT_PATH)	

find_library(LIB_SDL
    SDL2
    PATHS ${SDL_LIB_DIR}
    NO_DEFAULT_PATH)
	
add_executable(${PROJ_NAME}
	${CUR_DIR_SRCS}
	${CUR_DIR_HEADERS})
	
target_link_libraries(${PROJ_NAME}
	${LIB_AVCODEC}
	${LIB_AVFORMAT}
	${LIB_AVUTIL}
	${LIB_SWSCALE}
	${LIB_SWRESAMPLE}
	${LIB_SDL})