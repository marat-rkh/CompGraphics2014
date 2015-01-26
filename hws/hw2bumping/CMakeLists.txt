cmake_minimum_required(VERSION 2.8)

project(sample_0)

set(cpps main.cpp shader.cpp tiny_obj_loader.cc)
set(headers shader.h common.h utils.h  tiny_obj_loader.h)

IF (WIN32)
   set(EXTERNAL_LIBS ${PROJECT_SOURCE_DIR}/../../ext CACHE STRING "external libraries location")

   include_directories(${EXTERNAL_LIBS}/AntTweakBar/include)
   include_directories(${EXTERNAL_LIBS}/freeglut/include)
   include_directories(${EXTERNAL_LIBS}/glew-1.10.0/include)
   include_directories(${EXTERNAL_LIBS}/glm)

   link_directories(${EXTERNAL_LIBS}/AntTweakBar/lib)
   link_directories(${EXTERNAL_LIBS}/freeglut/lib)
   link_directories(${EXTERNAL_LIBS}/glew-1.10.0/lib/Release/Win32)

   add_executable(main ${cpps} ${headers})

   target_link_libraries(main freeglut glew32 AntTweakBar)

   add_custom_command(TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${EXTERNAL_LIBS}/AntTweakBar/lib"
        $<TARGET_FILE_DIR:main>)
   add_custom_command(TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${EXTERNAL_LIBS}/glew-1.10.0/bin/Release/Win32"
        $<TARGET_FILE_DIR:main>)
   add_custom_command(TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${EXTERNAL_LIBS}/freeglut/bin"
        $<TARGET_FILE_DIR:main>)

   add_custom_command(TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/shaders"
        ${PROJECT_BINARY_DIR}/shaders)
   add_custom_command(TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/shaders"
        $<TARGET_FILE_DIR:main>/shaders)
ELSE (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -Wall")

    add_executable(main ${cpps} ${headers})
	

    find_package(OpenGL REQUIRED)
    find_package(GLUT REQUIRED)

    find_package(GLEW REQUIRED)
    include_directories(${GLEW_INCLUDE_DIRS})
    link_directories(${GLEW_LIBRARY_DIRS})
    add_definitions(${GLEW_DEFINITIONS})
    if(NOT GLEW_FOUND)
       message(Error "GLEW not found")
    endif(NOT GLEW_FOUND)


    IF (FreeImage_LIBRARIES AND FreeImage_INCLUDE_DIRS)
        SET(FreeImage_FIND_QUIETLY TRUE) # Already in cache, be silent
    ELSE(FreeImage_LIBRARIES AND FreeImage_INCLUDE_DIRS)
        MESSAGE(STATUS "Looking for FreeImage")
    ENDIF (FreeImage_LIBRARIES AND FreeImage_INCLUDE_DIRS)
     
    SET(FreeImage_INCLUDE_SEARCH_DIRS
        ${FreeImage_LIBRARY_SEARCH_DIRS}
        ${CMAKE_LIBRARY_PATH}
        /usr/include
        /usr/local/include
        /opt/include
        /opt/freeimage/include
    )
     
    SET(FreeImage_LIBRARY_SEARCH_DIRS
        ${FreeImage_LIBRARY_SEARCH_DIRS}
        ${CMAKE_LIBRARY_PATH}
        /usr/lib
        /usr/local/lib
        /opt/lib
        /opt/freeimage/lib
    )
     
    FIND_PATH(FreeImage_INCLUDE_DIRS FreeImage.h ${FreeImage_INCLUDE_SEARCH_DIRS})
    FIND_LIBRARY(FreeImage_LIBRARIES freeimage PATHS ${FreeImage_LIBRARY_SEARCH_DIRS})
     
    #Do some preparation
    SEPARATE_ARGUMENTS(FreeImage_INCLUDE_DIRS)
    SEPARATE_ARGUMENTS(FreeImage_LIBRARIES)
     
    MARK_AS_ADVANCED(FreeImage_INCLUDE_DIRS FreeImage_LIBRARIES FreeImage_LIBRARY_DIRS)
     
    IF (FreeImage_INCLUDE_DIRS AND FreeImage_LIBRARIES)
        SET(FreeImage_FOUND TRUE)
    ENDIF (FreeImage_INCLUDE_DIRS AND FreeImage_LIBRARIES)
     
    IF (FreeImage_FOUND)
        IF (NOT FreeImage_FIND_QUIETLY)
            MESSAGE(STATUS "  libraries : ${FreeImage_LIBRARIES} from ${FreeImage_LIBRARY_DIRS}")
            MESSAGE(STATUS "  includes  : ${FreeImage_INCLUDE_DIRS}")
        ENDIF (NOT FreeImage_FIND_QUIETLY)
    ELSE (FreeImage_FOUND)
        IF (FreeImage_FIND_REQUIRED)
            MESSAGE(FATAL_ERROR "Could not find FreeImage")
        ENDIF (FreeImage_FIND_REQUIRED)
    ENDIF (FreeImage_FOUND)


   include_directories( ${OPENGL_INCLUDE_DIRS}  ${GLUT_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS})
   target_link_libraries(main AntTweakBar X11 GL glut GLEW freeimage)
ENDIF (WIN32)
