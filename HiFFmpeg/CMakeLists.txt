# CMakeList.txt: HiFFmpeg 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("HiFFmpeg")

# 指定 FFmpeg 安装目录，并将目录赋值给 FFMPEG_DEV_ROOT 变量，方便后续指令引用
set(FFMPEG_DEV_ROOT C:/Users/moli/.vmr/versions/ffmpeg_versions/ffmpeg/Library)

# 设置 FFmpeg 环境变量
# 指定 FFmpeg 头文件目录
include_directories(${FFMPEG_DEV_ROOT}/include)
# 指定链接器查找库文件  FFmpeg
link_directories(${FFMPEG_DEV_ROOT}/lib)
# 指定需要连接到程序中的库列表
link_libraries(
        avcodec
        avformat
        avfilter
        avdevice
        swresample
        swscale
        avutil
)

# 将源代码添加到此项目的可执行文件。
add_executable (HiFFmpeg "HiFFmpeg.cpp" "HiFFmpeg.h")

# 设置 CMake 构建目录
# 指定存放程序生成的静态库的目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# 指定存放程序生成的动态库的目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# 指定存放可执行软件的目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 复制动态库 dll 文件
# 查询 FFmpeg 安装目录下所有的动态库列表，并赋值给 ffmpeg_shared_libries 变量
file(GLOB ffmpeg_shared_libries ${FFMPEG_DEV_ROOT}/bin/*dll)
# 将 FFmpeg 动态库复制到 CMake 构建目录，运行程序需要用到
file(COPY ${ffmpeg_shared_libries} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})


if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET HiFFmpeg PROPERTY CXX_STANDARD 20)
endif()

# TODO: 如有需要，请添加测试并安装目标。
