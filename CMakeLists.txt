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
find_package(OpenGL REQUIRED)
list(APPEND IMGUI_SOURCES ${BAKENDS_FOLDER}imgui_impl_opengl3.cpp)
set(OPENGL_LIBRARIES OpenGL::GL)

list(APPEND IMGUI_SOURCES ${BAKENDS_FOLDER}imgui_impl_sdl2.cpp)

#sdl2
find_package(SDL2 REQUIRED)
get_target_property(SDL_INCLUDE SDL2::SDL2 INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "DIR: " ${SDL_INCLUDE})
include_directories(${SDL_INCLUDE})
set(IMGUI_SDL_LIBRARY SDL2::SDL2)
set(SDL_MAIN SDL2::SDL2main)

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
	src/mappers/mapper_004.c
	src/mappers/mapper_009.c
	src/mappers/mapper_007.c
)

target_include_directories(BudgetNES PRIVATE includes/ includes/mappers glad_loader/include/glad)

target_compile_definitions(BudgetNES PUBLIC -DCIMGUI_USE_OPENGL3 -DCIMGUI_USE_SDL2)
if (MINGW)
   target_link_options(BudgetNES PRIVATE "-mconsole")
endif()

add_library(glad_loader STATIC glad_loader/src/glad.c)
target_include_directories(glad_loader PRIVATE glad_loader/include/glad)

add_subdirectory(nativefiledialog-extended)

add_subdirectory(cglm EXCLUDE_FROM_ALL)
target_include_directories(cglm_headers INTERFACE cglm/include/cglm)

add_library(blip_buffer STATIC
	lib/CBlip_Buffer/CBlip_buffer.cpp
	lib/CBlip_Buffer//Blip_buffer/Blip_Buffer.cpp
)
target_include_directories(blip_buffer PUBLIC
	lib/CBlip_Buffer
	lib/CBlip_Buffer/Blip_Buffer
)

target_link_libraries(BudgetNES PRIVATE ${SDL_MAIN} cimgui_sdl glad_loader nfd cglm_headers blip_buffer)

if (UNIX)
target_link_libraries(BudgetNES PRIVATE "-framework UniformTypeIdentifiers -framework AppKit")
endif(UNIX)

if (MSVC)
	target_compile_options(BudgetNES PRIVATE /W4)
else(MSVC)
	target_compile_options(BudgetNES PRIVATE -Wall -Wextra -Wpedantic)
endif(MSVC)
