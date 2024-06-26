cmake_minimum_required(VERSION 3.28)
project(BudgetNES_Emulator)

set (CMAKE_CXX_STANDARD 11)

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

if (WIN32)
   add_definitions("-DIMGUI_IMPL_API=extern \"C\" __declspec\(dllexport\)")
else(WIN32)
   add_definitions("-DIMGUI_IMPL_API=extern \"C\" ")
endif(WIN32)

#openGL
set(OPENGL_LIBRARIES)
find_package(OpenGL)

if(OPENGL_FOUND)
   message(STATUS "openg gl found")
   list(APPEND IMGUI_SOURCES ${BAKENDS_FOLDER}imgui_impl_opengl3.cpp)
   set(OPENGL_LIBRARIES OpenGL::GL)
endif(OPENGL_FOUND)

#sdl2
list(APPEND IMGUI_SOURCES ${BAKENDS_FOLDER}imgui_impl_sdl2.cpp)
if(DEFINED SDL_PATH)
	message(STATUS "SDL_PATH defined as " ${SDL_PATH})
	FIND_PACKAGE(SDL2 PATHS ${SDL_PATH})
else(DEFINED SDL_PATH)
	# If SDL_PATH is not set, fallback and attempt to find SDL cmake script at a default location
	find_package(SDL2)
endif(DEFINED SDL_PATH)

if(SDL2_FOUND)
	get_target_property(SDL_INCLUDE SDL2::SDL2 INTERFACE_INCLUDE_DIRECTORIES)
	message(STATUS "sdl include is " ${SDL_INCLUDE})
   include_directories(${SDL_INCLUDE})
   set(IMGUI_SDL_LIBRARY SDL2::SDL2)
   set(SDL_MAIN SDL2::SDL2main)
   message(STATUS ${SDL_MAIN} ${IMGUI_SDL_LIBRARY})
else(SDL2_FOUND)
	if(DEFINED SDL_PATH)
		message(FATAL_ERROR " Cannot find SDL at SDL_PATH")
	else(DEFINED SDL_PATH)
		message(FATAL_ERROR " Cannot find SDL. Maybe try specifying SDL_PATH?")
	endif(DEFINED SDL_PATH)
endif(SDL2_FOUND)

add_library(cimgui_sdl SHARED ${IMGUI_SOURCES})
target_link_libraries(cimgui_sdl ${OPENGL_LIBRARIES} ${IMGUI_SDL_LIBRARY})

#using library
include_directories(cimgui/generator/output/)
add_executable(BudgetNES
   main.c
   src/display.c
   src/cpu.c
   src/ppu.c
   src/ppu_renderer_lookup.c
   src/apu.c
   src/cartridge.c
   src/controllers.c
   src/bus.c
   src/disassembler.c
   src/log.c
   src/mapper.c
   src/mappers/mirror_config.c
   src/mappers/mapper_000.c
   src/mappers/mapper_001.c
   src/mappers/mapper_002.c
)

find_package(cglm CONFIG)

if (cglm_FOUND)
   get_target_property(cglm_include cglm::cglm INTERFACE_INCLUDE_DIRECTORIES)
   set(cglm_library cglm::cglm)
   message(STATUS " cglm package found.")
else(cglm_FOUND)
   message(STATUS " cglm packge not found, provide path to cglm include with -Dcglm_include=path_to_include/include")
   find_library(cglm_library
      NAMES cglm
      HINTS /usr/local/lib
   )
endif(cglm_FOUND)

target_include_directories(BudgetNES PRIVATE includes/ glad_loader/include/glad ${cglm_include}/cglm)
target_compile_definitions(BudgetNES PUBLIC -DCIMGUI_USE_OPENGL3 -DCIMGUI_USE_SDL2)
if (MINGW)
   target_link_options(BudgetNES PRIVATE "-mconsole")
endif()

add_library(glad_loader SHARED glad_loader/src/glad.c)
target_include_directories(glad_loader PRIVATE glad_loader/include/glad)

add_subdirectory(nativefiledialog-extended)
target_link_libraries(BudgetNES PRIVATE ${SDL_MAIN} cimgui_sdl ${cglm_library} glad_loader nfd)

if (UNIX)
  target_link_libraries(BudgetNES PRIVATE "-framework UniformTypeIdentifiers -framework AppKit")
endif(UNIX)

target_compile_options(BudgetNES PRIVATE -Wall -Wextra -Wpedantic)
