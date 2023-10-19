cmake_minimum_required(VERSION 3.22)

project(gist 
	VERSION 0.1
	DESCRIPTION "A real-time client-server chat application using tcp sockets ")

add_executable(gist 
	src/client.c
	src/server.c
	src/utils.c
	src/logger.c)

target_include_directories(gist PRIVATE include)
