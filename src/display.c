#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "cimgui_impl.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include "display.h"

static SDL_Window* window = NULL;
static SDL_GLContext gContext = NULL;
static ImGuiIO* io = NULL;

static ImVec4 clear_color = {0.45f, 0.55f, 0.60f, 1.00f};

// gui components

static void display_demo();
static void display_main_viewport();

bool display_gui_init()
{
   // sdl init
   if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) < 0 )
   {
      SDL_Log("Failed to init: %s\n", SDL_GetError());
      return false;
   }

   const char*glsl_version = "#version 130";

   SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

   // setup window
   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
   SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
   SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

   SDL_WindowFlags window_flags = (SDL_WindowFlags) (SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
   window = SDL_CreateWindow("cimgui demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);

   if (window == NULL)
   {
      SDL_Log("Error, failed to create window: %s\n", SDL_GetError());
      return false;
   }

   // create opengl context

   gContext = SDL_GL_CreateContext(window);

   if (gContext == NULL)
   {
      SDL_Log("Error creating opengl context: %s\n", SDL_GetError());
      return false;
   }

   SDL_GL_MakeCurrent(window, gContext);
   SDL_GL_SetSwapInterval(1); // enable vsync
   SDL_Log("opengl version: %s", (char*)glGetString(GL_VERSION));

   // setup imgui context
   igCreateContext(NULL);

   io = igGetIO();
   io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // enable keyboard
   io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // enable docking
   io->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // enable  multi-viewport

   igStyleColorsDark(NULL); // set dark theme imgui style

   ImGui_ImplSDL2_InitForOpenGL(window, gContext);
   ImGui_ImplOpenGL3_Init(glsl_version);

   return true;
}

void display_gui_render()
{
   igRender();
   glViewport(0, 0, (int) io->DisplaySize.x, (int) io->DisplaySize.y);
   glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
   glClear(GL_COLOR_BUFFER_BIT);

   ImGui_ImplOpenGL3_RenderDrawData( igGetDrawData() );

   if ( io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
   {
      SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
      SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
      igUpdatePlatformWindows();
      igRenderPlatformWindowsDefault(NULL,NULL);
      SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
   }
}

void display_update()
{
   SDL_GL_SwapWindow(window);
}

void display_gui_shutdown()
{
   ImGui_ImplOpenGL3_Shutdown();
   ImGui_ImplSDL2_Shutdown();
   igDestroyContext(NULL);

   SDL_GL_DeleteContext(gContext);
   SDL_DestroyWindow(window);
   SDL_Quit();
}

void display_process_event(bool* done)
{
   static SDL_Event event;

   while ( SDL_PollEvent(&event) )
   {
      ImGui_ImplSDL2_ProcessEvent(&event);
      if ( event.type == SDL_QUIT )
      {
         *done = true;
      }
      //if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
      //{
      //   *done = true;
      //}
   }
}

void display_gui()
{
   ImGui_ImplOpenGL3_NewFrame();
   ImGui_ImplSDL2_NewFrame();
   igNewFrame();

   display_demo();
   display_main_viewport();
}

static void display_demo()
{
   static bool show_demo_window = true;
   static bool show_another_window = false;

   if ( show_demo_window )
   {
      igShowDemoWindow(&show_demo_window);
   }

   static float f = 0.0f;
   static int counter = 0;
   
   igBegin("Hello dummy!", NULL, 0);

   igText("This is some useful text");
   igCheckbox("Demo Window", &show_demo_window);
   igCheckbox("Another window", &show_another_window);

   igSliderFloat("Float", &f, 0.0f, 1.0f, "%.3f", 0);
   igColorEdit3("clear color", (float*) &clear_color, 0);

   ImVec2 button = {0, 0};
   if ( igButton("Button", button) )
   {
      counter++;
   }
   igSameLine(0.0f, -1.0f);
   igText("Counter = %d", counter);

   igText("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / igGetIO()->Framerate, igGetIO()->Framerate);
   igEnd();

   if (show_another_window)
   {
      igBegin("imgui Another Window", &show_another_window, ImGuiWindowFlags_NoDocking);
      igText("Hello from imgui");
      ImVec2 buttonSize;
      buttonSize.x = 0; buttonSize.y = 0;
      if (igButton("Close me", buttonSize))
      {
         show_another_window = false;
      }
      igEnd();
   }
}

static void display_main_viewport()
{
   static bool show_main_viewport = true;

   if (show_main_viewport)
   {
      igBegin("Viewport", &show_main_viewport, 0);

      igText("This is the main viewport");

      igEnd();
   }
}