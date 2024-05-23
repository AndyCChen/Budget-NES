#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "cimgui_impl.h"
#include "glad.h"
#include "SDL.h"
#include "SDL_opengl.h"
#include "cglm.h"
#include <stdint.h>

#include "../includes/display.h"
#include "../includes/cpu.h"
#include "../includes/log.h"
#include "../includes/controllers.h"
#include "../includes/ppu.h"

#define NES_PIXELS_W 256
#define NES_PIXELS_H 240
#define DISPLAY_W 256.0f * 3
#define DISPLAY_H 240.0f * 3

static SDL_Window* window = NULL;
static SDL_GLContext gContext = NULL;
static ImGuiIO* io = NULL;

static ImVec4 clear_color = {0.0f, 0.0f, 0.0f, 1.0f};

// gui components
// ---------------------------------------------------------------

static void gui_demo(void);
static void gui_main_viewport(void);
static void gui_cpu_debug(void);
static void gui_pattern_table_viewer(void);
static void gui_help_marker(const char* desc);

// opengl graphics
// ---------------------------------------------------------------

static GLuint viewport_FBO, pattern_table_FBO;      
static GLuint viewport_textureID, pattern_table_textureID;
static GLuint viewport_RBO, pattern_table_RBO;       
static GLuint viewport_VAO, pattern_table_VAO;
static GLuint viewport_color_VBO;

static vec4 viewport_pixel_colors[NES_PIXELS_H * NES_PIXELS_W];

static void display_add_shader(GLuint program, const GLchar* shader_code, GLenum type);
static bool display_init_main_viewport_buffers(void);
static bool display_init_pattern_table_buffers(void);
static bool display_create_shaders(void);
static bool display_create_frameBuffers(GLuint *FBO_ID, GLuint *textureID, GLuint *RBO_ID, int width, int height);
static void display_resize_framebuffer(int width, int height, GLuint textureID, GLuint RBO_ID);
static void display_set_pixel_position(float pixel_w, float pixel_h, mat4 pixel_pos[], uint32_t width, uint32_t height);

// global state of emulator
static Emulator_State_t emulator_state = 
{
   .display_size = 4,
   .cpu_debug = false,
   .pattern_table_viewer = false,
   .run_state = EMULATOR_RUNNING,
   .instruction_step = false,
}; 

/**
 * Returns pointer to the emulator state struct
*/
Emulator_State_t* get_emulator_state(void)
{
   return &emulator_state;
}

/**
 * setup sdl, create window, and opengl context
 * set ImGui io config flags
 * \returns true on success and false on failure
*/
bool display_init(void)
{
   // sdl init
   if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) < 0 )
   {
      SDL_Log("Failed to init: %s\n", SDL_GetError());
      return false;
   }

   #if __APPLE__
      const char* glsl_version = "#version 330";
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
   #else
      const char*glsl_version = "#version 330";
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
   #endif
   
   // setup window
   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
   SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
   SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

   SDL_WindowFlags window_flags = (SDL_WindowFlags) (SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN);
   window = SDL_CreateWindow("Budget NES Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, DISPLAY_W, DISPLAY_H, window_flags);

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

   // initialize glad loader
   if ( !gladLoadGLLoader( (GLADloadproc) SDL_GL_GetProcAddress ) )
   {
      printf("Failed to initialize GLAD\n");
      return false;
   }

   SDL_GL_MakeCurrent(window, gContext);
   SDL_GL_SetSwapInterval(-1); // enable vsync
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

   
   if ( !display_create_shaders() || !display_init_main_viewport_buffers() || !display_init_pattern_table_buffers() )
   {
      return false;
   }

   SDL_DisplayMode mode;
   if ( SDL_GetDesktopDisplayMode(0, &mode) != 0)
   {
      printf("Failed getting display information! %s\n", SDL_GetError());
   }

   emulator_state.refresh_rate = mode.refresh_rate;

   return true;
}

/**
 * call functions to shutdown and free all resources
*/
void display_shutdown(void)
{
   ImGui_ImplOpenGL3_Shutdown();
   ImGui_ImplSDL2_Shutdown();
   igDestroyContext(NULL);

   SDL_GL_DeleteContext(gContext);
   SDL_DestroyWindow(window);
   SDL_Quit();
}

/**
 * process sdl events
*/
void display_process_event(bool* done)
{
   SDL_Event event;

   while ( SDL_PollEvent(&event) )
   {
      ImGui_ImplSDL2_ProcessEvent(&event);

      if ( event.type == SDL_QUIT )
      {
         *done = true;
      }

      if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
      {
         *done = true;
      }

      if ( event.type == SDL_KEYUP )
      {
         switch (event.key.keysym.scancode)
         {
            // step through single instruction when period keypad is pressed and only when emulator is paused
            case SDL_SCANCODE_PERIOD:
            {
               if ( emulator_state.run_state == EMULATOR_PAUSED )
               {
                  emulator_state.instruction_step = true;
               }
               break;
            }
            // press P to pause emulator
            case SDL_SCANCODE_P:
            {
               emulator_state.run_state = (emulator_state.run_state == EMULATOR_RUNNING) ? EMULATOR_PAUSED : EMULATOR_RUNNING;
               break;
            }

            // gameplay controls

            case SDL_SCANCODE_W: // up
            {
               controller1_set_button_up(BUTTON_UP);
               break;
            }
            case SDL_SCANCODE_A: // left
            {
               controller1_set_button_up(BUTTON_LEFT);
               break;
            }
            case SDL_SCANCODE_S: // down
            {
               controller1_set_button_up(BUTTON_DOWN);
               break;
            }
            case SDL_SCANCODE_D: // right
            {
               controller1_set_button_up(BUTTON_RIGHT);
               break;
            }
            case SDL_SCANCODE_Q: // start
            {
               controller1_set_button_up(BUTTON_START);
               break;
            }
            case SDL_SCANCODE_E: // select
            {
               controller1_set_button_up(BUTTON_SELECT);
               break;
            }
            case SDL_SCANCODE_K: // B button
            {
               controller1_set_button_up(BUTTON_B);
               break;
            }
            case SDL_SCANCODE_L: // A button
            {
               controller1_set_button_up(BUTTON_A);
               break;
            } 

            default:
               break;
         }
      }

      if ( event.type == SDL_KEYDOWN )
      {
         switch (event.key.keysym.scancode)
         {
            // gameplay controls

            case SDL_SCANCODE_W: // up
            {
               controller1_set_button_down(BUTTON_UP);
               break;
            }
            case SDL_SCANCODE_A: // left
            {
               controller1_set_button_down(BUTTON_LEFT);
               break;
            }
            case SDL_SCANCODE_S: // down
            {
               controller1_set_button_down(BUTTON_DOWN);
               break;
            }
            case SDL_SCANCODE_D: // right
            {
               controller1_set_button_down(BUTTON_RIGHT);
               break;
            }
            case SDL_SCANCODE_Q: // start
            {
               controller1_set_button_down(BUTTON_START);
               break;
            }
            case SDL_SCANCODE_E: // select
            {
               controller1_set_button_down(BUTTON_SELECT);
               break;
            }
            case SDL_SCANCODE_K: // B button
            {
               controller1_set_button_down(BUTTON_B);
               break;
            }
            case SDL_SCANCODE_L: // A button
            {
               controller1_set_button_down(BUTTON_A);
               break;
            }
            default:
               break;
         }
      }
   }
}

void display_clear(void)
{
   //glViewport(0, 0, (int) io->DisplaySize.x, (int) io->DisplaySize.y);
   glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
   glClear(GL_COLOR_BUFFER_BIT);
}

/**
 * render graphics
*/
void display_render(void)
{
   // begin frame
   ImGui_ImplOpenGL3_NewFrame();
   ImGui_ImplSDL2_NewFrame();
   igNewFrame();

   if (emulator_state.cpu_debug) gui_cpu_debug();
   gui_demo();
   
   // send the updated color values buffer for pixels all at once
   glBindBuffer(GL_ARRAY_BUFFER, viewport_color_VBO);
   glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec4) * NES_PIXELS_W * NES_PIXELS_H, &viewport_pixel_colors);
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   gui_main_viewport();

   glBindFramebuffer(GL_FRAMEBUFFER, viewport_FBO);
   glBindVertexArray(viewport_VAO);
   glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0, NES_PIXELS_W * NES_PIXELS_H);
   glBindFramebuffer(GL_FRAMEBUFFER, 0);

   if (emulator_state.pattern_table_viewer) 
   {
      gui_pattern_table_viewer();
      glBindFramebuffer(GL_FRAMEBUFFER, pattern_table_FBO);
      glBindVertexArray(pattern_table_VAO);
      glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0, 128 * 128);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
   }
}

// update frame
void display_update(void)
{
   igRender();
   ImGui_ImplOpenGL3_RenderDrawData( igGetDrawData() );

   if ( io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
   {
      SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
      SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
      igUpdatePlatformWindows();
      igRenderPlatformWindowsDefault(NULL,NULL);
      SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
   }

   SDL_GL_SwapWindow(window);
}

static void gui_demo(void)
{
   static bool show_demo_window = false;
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

   igText("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io->Framerate, io->Framerate);
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

static void gui_main_viewport(void)
{
   ImVec2 zero_vec = {0, 0}; 

   ImGuiViewport* viewport = igGetMainViewport();
   igSetNextWindowPos(viewport->WorkPos, 0, zero_vec);
   igSetNextWindowSize(viewport->WorkSize, 0);
   igSetNextWindowViewport(viewport->ID);
   
   igPushStyleVar_Float(ImGuiStyleVar_WindowRounding, 0.0f);
   igPushStyleVar_Float(ImGuiStyleVar_WindowBorderSize, 0.0f);
   igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, zero_vec);  

   igBegin("NES", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking);
      igPopStyleVar(3);

      if (igBeginMenuBar())
      {
         if (igBeginMenu("File", true))
         {
            if ( igMenuItem_Bool("Load Rom", "Ctrl-L", false, true) )
            {
               // todo
            }

            if ( igMenuItem_Bool("Unload Rom", "Ctrl-U", false, true) )
            {
               // todo
            }

            if ( igMenuItem_Bool("Exit", "Esc", false, true) )
            {
               SDL_Event event;
               event.type = (SDL_EventType) SDL_QUIT;
               SDL_PushEvent(&event);
            }

            igEndMenu();
         }

         if (igBeginMenu("Tools", true))
         {
            if ( igMenuItem_Bool("CPU Debug", "", emulator_state.cpu_debug, true) )
            {
               emulator_state.cpu_debug = !emulator_state.cpu_debug;
            }

            if ( igMenuItem_Bool("Pattern Table Viewer", "", emulator_state.pattern_table_viewer, true) )
            {
               emulator_state.pattern_table_viewer = !emulator_state.pattern_table_viewer;
            }

            igEndMenu();
         }

         if (igBeginMenu("Window", true))
         {
            // use bit masking to toggle display size settings
            if ( igMenuItem_Bool("Fullscreen", "", emulator_state.display_size & 1, true) )
            {
               SDL_DisplayMode display_mode;
               if ( SDL_GetDesktopDisplayMode(0, &display_mode) != 0 )
               {
                  printf("Error in getting desktop display mode: %s\n", SDL_GetError());
               }

               SDL_SetWindowDisplayMode(window, &display_mode);
               SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
               
               printf("%d %d\n", display_mode.w, display_mode.h);
               emulator_state.display_size = (uint8_t) ~0xFE;
               io->ConfigFlags ^= ImGuiConfigFlags_ViewportsEnable; // disable multiviewports when in fullscreen
            }

            if ( igMenuItem_Bool("1x", "", emulator_state.display_size & 2, true) )
            {
               float w = DISPLAY_W - (NES_PIXELS_W);
               float h = DISPLAY_H - (NES_PIXELS_H);

               SDL_SetWindowFullscreen(window, 0);
               SDL_SetWindowSize( window, w, h );
               SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
               emulator_state.display_size = (uint8_t) ~0xFD;
               io->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
            }

            if ( igMenuItem_Bool("2x", "", emulator_state.display_size & 4, true) )
            {
               float w = DISPLAY_W;
               float h = DISPLAY_H;

               SDL_SetWindowFullscreen(window, 0);
               SDL_SetWindowSize( window, w, h );
               SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
               emulator_state.display_size = (uint8_t) ~0xFB;
               io->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
            }

            if ( igMenuItem_Bool("3x", "", emulator_state.display_size & 8, true) )
            {
               float w = DISPLAY_W + (NES_PIXELS_W);
               float h = DISPLAY_H + (NES_PIXELS_H);

               SDL_SetWindowFullscreen(window, 0);
               SDL_SetWindowSize( window, w, h );
               SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
               emulator_state.display_size = (uint8_t) ~0xF7;
               io->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
            }

            igEndMenu();
         }
      
         igEndMenuBar();
      }

      igBeginChild_Str("Child NES Viewport", zero_vec, ImGuiChildFlags_None, ImGuiWindowFlags_None);
         ImVec2 size;
         igGetWindowSize(&size);
         glViewport(0, 0, (int) size.x, (int) size.y);
         display_resize_framebuffer((int) size.x, (int) size.y, viewport_textureID, viewport_RBO);
         ImVec2 uv_min = {0, 1};
         ImVec2 uv_max = {1, 0};
         ImVec4 col = {1, 1, 1, 1};
         ImVec4 border = {0, 0, 0, 0};
         igImage( (void*) (uintptr_t) viewport_textureID, size, uv_min, uv_max, col, border); 
      igEndChild();
   igEnd();
}

static void gui_cpu_debug(void)
{
   cpu_6502_t* cpu = get_cpu();
   ImVec4 red = {0.9686274509803922f, 0.1843137254901961f, 0.1843137254901961f, 1.0f};
   ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerV;
   ImVec2 zero_vec = {0.0f, 0.0f};

   igBegin("CPU Debug", &emulator_state.cpu_debug, ImGuiWindowFlags_None);
      igText("CPU Registers");
      if ( igBeginTable("CPU registers table", 2, flags, zero_vec, 0.0f) )
      {
         igTableNextRow(0, 0.0f);
         igTableSetColumnIndex(0);

         igTextColored(red, "PC: ");
         igSameLine(0.0f, -1.0f);
         igText("%04X", cpu->pc);
         gui_help_marker("Program counter");

         igTextColored(red, " A: ");
         igSameLine(0.0f, -1.0f);
         igText("  %02X", cpu->ac);
         gui_help_marker("Acumulator");

         igTextColored(red, " X: ");
         igSameLine(0.0f, -1.0f);
         igText("  %02X", cpu->X);
         gui_help_marker("X register");

         igTextColored(red, " Y: ");
         igSameLine(0.0f, -1.0f);
         igText("  %02X", cpu->Y);
         gui_help_marker("Y register");

         igTextColored(red, "SP: ");
         igSameLine(0.0f, -1.0f);
         igText("  %02X", cpu->sp);
         gui_help_marker("Stack pointer");

         igTableSetColumnIndex(1);

         igTextColored(red, " P: ");
         igSameLine(0.0f, -1.0f);
         igText("  %02X", cpu->status_flags);
         gui_help_marker("CPU processor flags, below are the individual bits representing each status flag");

         igTextColored(red, " C: ");
         igSameLine(0.0f, -1.0f);
         igText("   %1X", cpu->status_flags & 1);
         gui_help_marker("Carry Flag");

         igTextColored(red, " Z: ");
         igSameLine(0.0f, -1.0f);
         igText("   %1X", (cpu->status_flags & 2) >> 1);
         gui_help_marker("Zero Flag");

         igTextColored(red, " I: ");
         igSameLine(0.0f, -1.0f);
         igText("   %1X", (cpu->status_flags & 4) >> 2);
         gui_help_marker("Interrupt Flag");

         igTextColored(red, " D: ");
         igSameLine(0.0f, -1.0f);
         igText("   %1X", (cpu->status_flags & 8) >> 3);
         gui_help_marker("Binary decimal mode Flag");

         igTextColored(red, " B: ");
         igSameLine(0.0f, -1.0f);
         igText("   %1X", (cpu->status_flags & 16) >> 4);
         gui_help_marker("Break Flag");

         igTextColored(red, " -: ");
         igSameLine(0.0f, -1.0f);
         igText("   %1X", (cpu->status_flags & 32) >> 5);
         gui_help_marker("Unused Flag");

         igTextColored(red, " V: ");
         igSameLine(0.0f, -1.0f);
         igText("   %1X", (cpu->status_flags & 64) >> 6);
         gui_help_marker("Overflow Flag");

         igTextColored(red, " N: ");
         igSameLine(0.0f, -1.0f);
         igText("   %1X", (cpu->status_flags & 128) >> 7);
         gui_help_marker("Negative Flag");

         igEndTable();
      }
      igNewLine();

      igText("Instruction Log");
      gui_help_marker("Disassembly of program instructions.");

      if ( igBeginTable("Instruction Disassembly", 2, flags, zero_vec, 0.0f) )
      {
         igTableNextRow(0, 0.0f);
         igTableSetColumnIndex(0);

         log_update_current();

         // log previous disassembled opcodes
         igText("%s", log_get_prev_instruction(5));
         igText("%s", log_get_prev_instruction(4));
         igText("%s", log_get_prev_instruction(3));
         igText("%s", log_get_prev_instruction(2));
         igText("%s", log_get_prev_instruction(1));

         // log current opcode that will be executed
         igTextColored(red, "%s", log_get_current_instruction());

         // log future opcodes that will be executed
         igText("%s", log_get_next_instruction(1));
         igText("%s", log_get_next_instruction(2));
         igText("%s", log_get_next_instruction(3));
         igText("%s", log_get_next_instruction(4));
         igText("%s", log_get_next_instruction(5));

         // pause, instruction and frame step buttons
         igTableSetColumnIndex(1);

         igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, red);
         igPushStyleColor_Vec4(ImGuiCol_ButtonActive, red);

         if (emulator_state.run_state == EMULATOR_PAUSED) 
         {
            igPushStyleColor_Vec4(ImGuiCol_Button, red);
            if ( igButton("Pause", zero_vec) )
            {
               emulator_state.run_state = EMULATOR_RUNNING;
            }
            igPopStyleColor(1);
         }
         else
         {
            if ( igButton("Pause", zero_vec) )
            {
               emulator_state.run_state = EMULATOR_PAUSED;
            }
         }

         igBeginDisabled(emulator_state.run_state == EMULATOR_RUNNING);
            if ( igButton("Intruction Step", zero_vec) )
            {
               emulator_state.instruction_step = true;
            }
         igEndDisabled();
         gui_help_marker("Step through a single instruction while the emulator is paused. Has no effect when emulator is not paused.");

         igPopStyleColor(2);
         igEndTable();
      }
   igEnd();
}

static void gui_pattern_table_viewer(void)
{
   igBegin("Pattern Tables", &emulator_state.pattern_table_viewer, ImGuiWindowFlags_None);

      ImVec2 zero_vec = {0, 0};
      ImVec2 size; 
      igGetWindowSize(&size);

      igText("Pattern Table 0"); gui_help_marker("Table 1");
      igSameLine( (size.x / 2) + 5, -1.0f );
      igText("Pattern Table 1");
      
      igBeginChild_Str("Pattern Tables Container", zero_vec, ImGuiChildFlags_None, ImGuiWindowFlags_None);
         igGetWindowSize(&size);
         ImVec2 child_size = {(size.x / 2.0f) - 5, (size.x / 2.0f) - 5};   

         ImVec2 uv_min = {0, 1};
         ImVec2 uv_max = {1, 0};
         ImVec4 col = {1, 1, 1, 1};
         ImVec4 border = {0, 0, 0, 0};
         
         igBeginChild_Str("Pattern Table 0", child_size, ImGuiChildFlags_None, ImGuiWindowFlags_None);
            igGetWindowSize(&size);
            glViewport(0, 0, (int) child_size.x, (int) child_size.y);
            display_resize_framebuffer(child_size.x, child_size.y, pattern_table_textureID, pattern_table_RBO);
            igImage( (void*) (uintptr_t) pattern_table_textureID, child_size, uv_min, uv_max, col, border); 
         igEndChild();
         
         igSameLine(0.0f, -1.0f);

         igBeginChild_Str("Pattern Table 1", child_size, ImGuiChildFlags_None, ImGuiWindowFlags_None);
            igImage( (void*) (uintptr_t) pattern_table_textureID, child_size, uv_min, uv_max, col, border); 
         igEndChild();
      igEndChild();
   igEnd();
}

static bool display_init_main_viewport_buffers(void)
{
   int width, height;
   SDL_GetWindowSize(window, &width, &height);

   mat4* pixel_pos = malloc(NES_PIXELS_W * NES_PIXELS_W * sizeof(mat4));
   if (pixel_pos == NULL) printf("Failed to allocate memory for pixel pos\n");

   float pixel_w = (float) width / NES_PIXELS_W;
   float pixel_h = (float) height / NES_PIXELS_H;
   display_set_pixel_position(pixel_w, pixel_h, pixel_pos, NES_PIXELS_W, NES_PIXELS_H);

   float pixel_vertices[] = {
      0.0f,    0.0f,    0.0f, // top left
      0.0f,    pixel_h, 0.0f, // bottom left
      pixel_w, pixel_h, 0.0f, // bottom right
      pixel_w, 0.0f,    0.0f, // top right
   };

   GLubyte indices[] = {
      0, 1, 2,
      2, 3, 0,
   };

   // send vertex data for pixels
   GLuint viewport_pixel_VBO, viewport_pixel_IBO;
   glGenVertexArrays(1, &viewport_VAO);
   glGenBuffers(1, &viewport_pixel_VBO);
   glGenBuffers(1, &viewport_pixel_IBO);

   glBindVertexArray(viewport_VAO);
   glBindBuffer(GL_ARRAY_BUFFER, viewport_pixel_VBO);
   glBufferData(GL_ARRAY_BUFFER, sizeof(pixel_vertices), pixel_vertices, GL_STATIC_DRAW);

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, viewport_pixel_IBO);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);

   // send instanced transformation data for pixels
   GLuint transformation_VBO;
   glGenBuffers(1, &transformation_VBO);

   glBindBuffer(GL_ARRAY_BUFFER, transformation_VBO);
   glBufferData(GL_ARRAY_BUFFER, sizeof(mat4) * NES_PIXELS_W * NES_PIXELS_H, pixel_pos, GL_STATIC_DRAW);
   free(pixel_pos);

   glEnableVertexAttribArray(1);
   glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*) 0);
   glEnableVertexAttribArray(2);
   glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*) ( 1 * sizeof(vec4) ));
   glEnableVertexAttribArray(3);
   glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*) ( 2 * sizeof(vec4) ));
   glEnableVertexAttribArray(4);
   glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*) ( 3 * sizeof(vec4) ));

   glVertexAttribDivisor(1, 1);
   glVertexAttribDivisor(2, 1);
   glVertexAttribDivisor(3, 1);
   glVertexAttribDivisor(4, 1);

   // send instanced color data for pixels
   glGenBuffers(1, &viewport_color_VBO);
   glBindBuffer(GL_ARRAY_BUFFER, viewport_color_VBO);
   glBufferData(GL_ARRAY_BUFFER, sizeof(vec4) * NES_PIXELS_W * NES_PIXELS_H, viewport_pixel_colors, GL_DYNAMIC_DRAW);
   glEnableVertexAttribArray(5);
   glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), (void*) 0);
   
   glVertexAttribDivisor(5, 1);

   glBindVertexArray(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

   if ( !display_create_frameBuffers(&viewport_FBO, &viewport_textureID, &viewport_RBO, width, height) ) return false;

   return true;
}

static bool display_init_pattern_table_buffers(void)
{
   int width, height;
   SDL_GetWindowSize(window, &width, &height);

   mat4* pixel_pos = malloc(128 * 128 * sizeof(mat4));
   if (pixel_pos == NULL) printf("Failed to allocate memory for pixel pos\n");

   float pixel_w = (float) width / 128;
   float pixel_h = (float) height / 128;
   display_set_pixel_position(pixel_w, pixel_h, pixel_pos, 128, 128);

   float pixel_vertices[] = {
      0.0f,    0.0f,    0.0f, // top left
      0.0f,    pixel_h, 0.0f, // bottom left
      pixel_w, pixel_h, 0.0f, // bottom right
      pixel_w, 0.0f,    0.0f, // top right
   };

   GLubyte indices[] = {
      0, 1, 2,
      2, 3, 0,
   };

   GLuint pattern_table_VBO, pattern_table_IBO;
   glGenVertexArrays(1, &pattern_table_VAO);
   glGenBuffers(1, &pattern_table_VBO);
   glGenBuffers(1, &pattern_table_IBO);

   glBindVertexArray(pattern_table_VAO);
   glBindBuffer(GL_ARRAY_BUFFER, pattern_table_VBO);
   glBufferData(GL_ARRAY_BUFFER, sizeof(pixel_vertices), pixel_vertices, GL_STATIC_DRAW);

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pattern_table_IBO);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*) 0);

   GLuint transformation_VBO;
   glGenBuffers(1, &transformation_VBO);

   glBindBuffer(GL_ARRAY_BUFFER, transformation_VBO);
   glBufferData(GL_ARRAY_BUFFER, sizeof(mat4) * 128 * 128, pixel_pos, GL_STATIC_DRAW);
   free(pixel_pos);

   glEnableVertexAttribArray(1);
   glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*) 0);
   glEnableVertexAttribArray(2);
   glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*) ( 1 * sizeof(vec4) ));
   glEnableVertexAttribArray(3);
   glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*) ( 2 * sizeof(vec4) ));
   glEnableVertexAttribArray(4);
   glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*) ( 3 * sizeof(vec4) ));

   glVertexAttribDivisor(1, 1);
   glVertexAttribDivisor(2, 1);
   glVertexAttribDivisor(3, 1);
   glVertexAttribDivisor(4, 1);

   GLuint pattern_table_color_VBO;
   vec4* pattern_table_pixel_colors = malloc(sizeof(vec4) * 128 * 128);
   DEBUG_ppu_init_pattern_table(pattern_table_pixel_colors);

   /* for (int i = 0; i < 16384 - 1; ++i)
   {
      pattern_table_pixel_colors[i][0] = 0.2f;
      pattern_table_pixel_colors[i][1] = 0.5f;
      pattern_table_pixel_colors[i][2] = 0.8f;
      pattern_table_pixel_colors[i][3] = 1.0f;
   } */

   glGenBuffers(1, &pattern_table_color_VBO);
   glBindBuffer(GL_ARRAY_BUFFER, pattern_table_color_VBO);
   glBufferData(GL_ARRAY_BUFFER, sizeof(vec4) * 128 * 128, pattern_table_pixel_colors, GL_STATIC_DRAW);
   glEnableVertexAttribArray(5);
   glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), (void*) 0);

   glVertexAttribDivisor(5, 1);

   glBindVertexArray(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
   
   free(pattern_table_pixel_colors);

   if ( !display_create_frameBuffers(&pattern_table_FBO, &pattern_table_textureID, &pattern_table_RBO, width, height) ) return false;

   return true;
}

static bool display_create_frameBuffers(GLuint *FBO_ID, GLuint *textureID, GLuint *RBO_ID, int width, int height)
{
   glGenFramebuffers(1, FBO_ID);
   glBindFramebuffer(GL_FRAMEBUFFER, *FBO_ID);

   glGenTextures(1, textureID);
   glBindTexture(GL_TEXTURE_2D, *textureID);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA , width, height, 0, GL_RGBA , GL_UNSIGNED_BYTE, NULL);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glBindTexture(GL_TEXTURE_2D, 0);

   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *textureID, 0);

   glGenRenderbuffers(1, RBO_ID);
   glBindRenderbuffer(GL_RENDERBUFFER, *RBO_ID);
   glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
   glBindRenderbuffer(GL_RENDERBUFFER, 0);

   glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, *RBO_ID);

   
   if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
   {
      printf("Framebuffer creation failed!\n");
      return false;
   }
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   
   return true;
}

static void display_resize_framebuffer(int width, int height, GLuint textureID, GLuint RBO_ID)
{
   glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureID, 0);

	glBindRenderbuffer(GL_RENDERBUFFER, RBO_ID);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO_ID);
}

static bool display_create_shaders(void)
{
   GLchar* vertex_shader_code = "#version 330 core\n"
      "layout (location = 0) in vec3 aPos;\n"
      "layout (location = 1) in mat4 instanceMatrix;\n"
      "layout (location = 5) in vec4 instanceColor;\n"

      "out vec4 color;\n"

      "uniform mat4 view;\n"
      "uniform mat4 projection;\n"

      "void main()\n"
      "{\n"
      "   gl_Position = projection * view * instanceMatrix * vec4(aPos, 1.0);\n"
      "   color = instanceColor;\n"
      "}\0";

   GLchar* fragment_shader_code = "#version 330 core\n"
      "out vec4 fragColor;\n"
      "in vec4 color;\n"

      "void main()\n"
      "{\n"
      "  fragColor = color;\n"
      "}\0";

   GLuint shader_program = glCreateProgram();
   if (shader_program == 0)
   {
      printf("Error creating shader program!\n");
      return false;
   }

   display_add_shader(shader_program, vertex_shader_code, GL_VERTEX_SHADER);
   display_add_shader(shader_program, fragment_shader_code, GL_FRAGMENT_SHADER);

   glLinkProgram(shader_program);

   int success;
   char shader_prog_log[512];

   glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
   if (success == 0) // shader program linking failure
   {
      glGetProgramInfoLog(shader_program, sizeof(shader_prog_log), NULL, shader_prog_log);
      printf("shader program linker error: %s\n", shader_prog_log);
      return false;
   }

   glUseProgram(shader_program);

   mat4 view = GLM_MAT4_IDENTITY_INIT;
   vec3 translate = {0.0f, 0.0f, 0.0f};
   glm_translate(view, translate);
   GLuint viewLoc = glGetUniformLocation(shader_program, "view");
   glUniformMatrix4fv(viewLoc, 1, GL_FALSE, (GLfloat*) view);

   mat4 ortho_projection = GLM_MAT4_IDENTITY_INIT;
   glm_ortho(0.0f, DISPLAY_W, DISPLAY_H, 0.0f, -1.0f, 1.0f, ortho_projection);
   GLuint projectionLoc = glGetUniformLocation(shader_program, "projection");
   glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, (GLfloat*) ortho_projection);

   return true;
}

/**
 * create and compile a shader, also attaches the shader to a shader program
 * @param program program object id created from glCreateProgram
 * @param shader_code shader source code string
 * @param type type of shader to create, i.e: GL_VERTEX_SHADER
*/
static void display_add_shader(GLuint program, const GLchar* shader_code, GLenum type)
{
   GLuint shader = glCreateShader(type);
   if (shader == 0) // shader creation error
   {
      printf("shader creation error!\n");
      return;
   }

   glShaderSource(shader, 1, &shader_code, NULL);
   glCompileShader(shader);

   int success;
   char shader_log[512];

   glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
   if (success == 0) // shader compile error
   {
      glGetShaderInfoLog(shader, sizeof(shader_log), NULL, shader_log);
      printf("Shader compilation error: %s", shader_log);
      return;
   }

   glAttachShader(program, shader);
}

/**
 * Sets the position of each pixel of the nes display
 * @param w width of the pixel
 * @param h height of the pixel
*/
static void display_set_pixel_position(float pixel_w, float pixel_h, mat4 pixel_pos[], uint32_t width, uint32_t height)
{
   for (size_t row = 0; row < height; ++row)
   {
      for (size_t col = 0; col < width; ++col)
      {
         mat4 model = GLM_MAT4_IDENTITY_INIT;
         vec3 translation = {pixel_w * col, pixel_h * row, 0.0f};
         glm_translate(model, translation);
         glm_mat4_copy(model, pixel_pos[row * width + col]);
      }
   }
}

/**
 * Updates color of a pixel in the buffer on host memory side.
 * @param row of the pixel
 * @param col of the pixel
*/
void set_pixel_color(uint32_t row, uint32_t col, vec3 color)
{
   uint32_t index = row * NES_PIXELS_W + col;

   viewport_pixel_colors[index][0] = color[0];
   viewport_pixel_colors[index][1] = color[1];
   viewport_pixel_colors[index][2] = color[2];
   viewport_pixel_colors[index][3] = 1.0f;
}

static void gui_help_marker(const char* desc)
{
   igSameLine(0.0f, -1.0f);
   igTextDisabled("(?)");
   if (igBeginItemTooltip())
   {
      igPushTextWrapPos(igGetFontSize() * 15.0f);
      igTextUnformatted(desc, NULL);
      igPopTextWrapPos();
      igEndTooltip();
   }
}

uint8_t display_get_refresh_rate(void)
{
   return emulator_state.refresh_rate;
}
