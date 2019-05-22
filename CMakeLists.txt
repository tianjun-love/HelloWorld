cmake_minimum_required (VERSION 2.6) #cmake最低版本

project (HelloWorldLib) #项目名称

set(PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}) #自定义，项目根目录，子目录获取
set(PROJECT_DEPEND_DIR ${PROJECT_ROOT_DIR}) #项目依赖库根目录，自定义，子目标可获取
set(CMAKE_INSTALL_PREFIX ${PROJECT_ROOT_DIR}) #安装根目录，install需要

#目录不存在，则创建目录
file(MAKE_DIRECTORY ${PROJECT_ROOT_DIR}/lib ${PROJECT_ROOT_DIR}/bin)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	set(PROJECT_C_COMPILE_FLAG "-Wall -ggdb -O0 -std=c99") #自定义，C编译选项，子目录获取
	set(PROJECT_CXX_COMPILE_FLAG "-Wall -ggdb -O0 -std=c++11") #自定义，C++编译选项，子目录获取
	set(PROJECT_DEFINE_FLAG "-D_DEBUG") #自定义，宏定义选项，子目录获取
else()
	set(PROJECT_C_COMPILE_FLAG "-Wall -O3 -std=c99") #自定义，C编译选项，子目录获取
	set(PROJECT_CXX_COMPILE_FLAG "-Wall -O3 -std=c++11") #自定义，C++编译选项，子目录获取
	set(PROJECT_DEFINE_FLAG "-DNDEBUG") #自定义，宏定义选项，子目录获取
endif()

set(CMAKE_VERBOSE_MAKEFILE ON) #显示详细编译信息

add_subdirectory(DateTime) #编译库
add_subdirectory(LogFile) #编译库
add_subdirectory(XmlObject) #编译库
add_subdirectory(JsonObject) #编译库
add_subdirectory(test) #编译可执行程序