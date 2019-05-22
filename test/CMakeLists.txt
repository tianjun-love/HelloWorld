project (test) #项目名称

set(EXECUTABLE_FILE_NAME "") #输出目标文件名称

set(CMAKE_C_FLAGS ${PROJECT_C_COMPILE_FLAG}) #编译选项
set(CMAKE_CXX_FLAGS ${PROJECT_CXX_COMPILE_FLAG}) #编译选项
add_definitions(${PROJECT_DEFINE_FLAG}) #宏定义，以空格隔开
include_directories(${PROJECT_DEPEND_DIR}) #头文件搜索目录，以空格隔开
link_directories(${PROJECT_ROOT_DIR}/lib) #库文件搜索目录，以空格隔开
aux_source_directory(./src SRC_FILES) #获取目录下所有源文件

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	set(EXECUTABLE_FILE_NAME "test.d")
else()
	set(EXECUTABLE_FILE_NAME "test")
endif()

add_executable(${EXECUTABLE_FILE_NAME} ${SRC_FILES}) #编译成可执行文件
target_link_libraries(${EXECUTABLE_FILE_NAME} pthread) #添加链接库

install(TARGETS ${EXECUTABLE_FILE_NAME} RUNTIME DESTINATION bin) #安装