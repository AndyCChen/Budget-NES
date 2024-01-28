cmake_minimum_required(VERSION 2.8)
project(nes_emulator)

if(WIN32) #to mingw work as all the others
   set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif(WIN32)

# general settings
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

set(BAKENDS_FOLDER "cimgui/imgui/backends/")

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cimgui/imgui/imgui_tables.cpp)
	set(TABLES_SOURCE "cimgui/imgui/imgui_tables.cpp")
else()
	set(TABLES_SOURCE "")
endif()

include_directories(cimgui/imgui)
add_definitions("-DIMGUI_DISABLE_OBSOLETE_FUNCTIONS=1")

include_directories(cimgui)
set(IMGUI_SOURCES 
   cimgui/cimgui.cpp 
   cimgui/imgui/imgui.cpp 
   cimgui/imgui/imgui_draw.cpp 
   cimgui/imgui/imgui_demo.cpp 
   cimgui/imgui/imgui_widgets.cpp
   ${TABLES_SOURCE} 
)

set(IMGUI_LIBRARIES)

if (WIN32)
   add_definitions("-DIMGUI_IMPL_API=extern \"C\" __declspec\(dllexport\)")
else(WIN32)
   add_definitions("-DIMGUI_IMPL_API=extern \"C\" ")
endif(WIN32)

#opengl3
list(APPEND IMGUI_SOURCES ${BAKENDS_FOLDER}imgui_impl_opengl2.cpp)
if(WIN32)
   list(APPEND IMGUI_LIBRARIES opengl32)
else(WIN32)#Unix
   list(APPEND IMGUI_LIBRARIES GL)
endif(WIN32)

#sdl2
list(APPEND IMGUI_SOURCES ${BAKENDS_FOLDER}imgui_impl_sdl2.cpp)
if(DEFINED SDL_PATH)
	message(STATUS "SDL_PATH defined as " ${SDL_PATH})
	FIND_PACKAGE(SDL2 PATHS ${SDL_PATH})
else(DEFINED SDL_PATH)
	message(STATUS "FINDING PACKAGE")
	# If SDL_PATH is not set, fallback and attempt to find SDL cmake script at a default location
	find_package(SDL2)
endif(DEFINED SDL_PATH)

if(SDL2_FOUND)
	get_target_property(SDL_INCLUDE SDL2::SDL2 INTERFACE_INCLUDE_DIRECTORIES)
	message(STATUS "sdlinclude is " ${SDL_INCLUDE})
   include_directories(${SDL_INCLUDE})
   set(IMGUI_SDL_LIBRARY SDL2::SDL2)
   set(SDL_MAIN SDL2::SDL2main)
   message(STATUS ${SDL_MAIN} ${IMGUI_SDL_LIBRARY})
else(SDL2_FOUND)
	if(DEFINED SDL_PATH)
		message(FATAL_ERROR "Cannot find SDL at SDL_PATH")
	else(DEFINED SDL_PATH)
		message(FATAL_ERROR "Cannot find SDL. Maybe try specifying SDL_PATH?")
	endif(DEFINED SDL_PATH)
endif(SDL2_FOUND)

add_library(cimgui_sdl SHARED ${IMGUI_SOURCES})
target_link_libraries(cimgui_sdl ${IMGUI_LIBRARIES} ${IMGUI_SDL_LIBRARY})

#using library
include_directories(cimgui/generator/output/)
add_executable(nes 
   main.c
   src/bus.c
   src/cartridge.c
   src/cpu_ram.c
   src/cpu.c
   src/log.c
)
target_include_directories(nes PRIVATE includes/)
target_compile_definitions(nes PUBLIC -DCIMGUI_USE_OPENGL2 -DCIMGUI_USE_SDL2)
if (MINGW)
   target_link_options(nes PRIVATE "-mconsole")
endif()
target_link_libraries(nes ${IMGUI_SDL_LIBRARY} ${SDL_MAIN} cimgui_sdl)