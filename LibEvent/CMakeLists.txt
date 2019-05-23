project (LibEvent) #项目名称

set(CMAKE_C_FLAGS ${PROJECT_C_COMPILE_FLAG}) #编译选项
set(CMAKE_CXX_FLAGS ${PROJECT_CXX_COMPILE_FLAG}) #编译选项
add_definitions(${PROJECT_DEFINE_FLAG}) #宏定义，以空格隔开
include_directories(${PROJECT_DEPEND_DIR} ./libevent) #头文件搜索目录，以空格隔开
aux_source_directory(./src SRC_FILES) #获取目录下所有源文件

add_library(LibEvent SHARED ${SRC_FILES}) #编译成动态库

install(TARGETS LibEvent LIBRARY DESTINATION lib) #安装