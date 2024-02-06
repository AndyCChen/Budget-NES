#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "cimgui_impl.h"

#include "glad.h"
#include "SDL.h"
#include "SDL_opengl.h"

#include "display.h"

static SDL_Window* window = NULL;
static SDL_GLContext gContext = NULL;
static ImGuiIO* io = NULL;

static ImVec4 clear_color = {0.45f, 0.55f, 0.60f, 1.00f};

// gui components
// ---------------------------------------------------------------

static void display_gui_demo();
static void display_gui_display();

// opengl graphics
// ---------------------------------------------------------------

static GLuint VBO[2];
static GLuint VAO[2];
static GLuint shader_program;

static void add_shader(GLuint program, const GLchar* shader_code, GLenum type);

/**
 * setup sdl, create window, and opengl context
 * set ImGui io config flags
 * \returns true on success and false on failure
*/
bool display_init()
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

   // initialize glad loader
   if ( !gladLoadGLLoader( (GLADloadproc) SDL_GL_GetProcAddress ) )
   {
      printf("Failed to initialize GLAD\n");
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

/**
 * render imgui components
*/
void display_render()
{
   glUseProgram(shader_program);

   float timeValue = SDL_GetTicks64() / 1000.0f;
   float greenValue = (sin(timeValue) / 2.0f) + 0.5f;
   int vertexColorLocation = glGetUniformLocation(shader_program, "ourColor");
   glUniform4f(vertexColorLocation, 0.0f, greenValue, 0.0f, 1.0f);

   glBindVertexArray(VAO[0]);
   glDrawArrays(GL_TRIANGLES, 0, 3);
   
   glBindVertexArray(VAO[1]);
   glDrawArrays(GL_TRIANGLES, 0, 3);

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
}

/**
 * updates the screen
*/
void display_update()
{
   SDL_GL_SwapWindow(window);
}

/**
 * call functions to shutdown and free all resources
*/
void display_shutdown()
{
   ImGui_ImplOpenGL3_Shutdown();
   ImGui_ImplSDL2_Shutdown();
   igDestroyContext(NULL);

   SDL_GL_DeleteContext(gContext);
   SDL_DestroyWindow(window);
   SDL_Quit();
}

/**
 * create imgui components
*/
void display_create_gui()
{
   glViewport(0, 0, (int) io->DisplaySize.x, (int) io->DisplaySize.y);
   glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
   glClear(GL_COLOR_BUFFER_BIT);

   ImGui_ImplOpenGL3_NewFrame();
   ImGui_ImplSDL2_NewFrame();
   igNewFrame();

   display_gui_demo();
   display_gui_display();
}

/**
 * process sdl events
*/
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
      if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
      {
         *done = true;
      }
   }
}

static void display_gui_demo()
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

/**
 * create the gui for the main display viewport
*/
static void display_gui_display()
{
   static bool show_main_viewport = true;

   if (show_main_viewport)
   {
      igBegin("Viewport", &show_main_viewport, 0);
      igText("This is the main viewport");

      igEnd();
   }
}

void graphics_create_triangle()
{
   float triangle1[] = {
      // position          // colors
      -0.9f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,
      -0.0f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,
      -0.45f, 0.5f, 0.0f,  0.0f, 0.0f, 1.0f
   };

   float triangle2[] = {
      // position         // colors
      0.0f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,
      0.9f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,
      0.45f, 0.5f, 0.0f,   0.0f, 0.0f, 1.0f
   };

   glGenVertexArrays(2, VAO);
   glGenBuffers(2, VBO);

   glBindVertexArray(VAO[0]);
   glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
   glBufferData(GL_ARRAY_BUFFER, sizeof(triangle1), triangle1, GL_STATIC_DRAW);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) 0);
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) (3 * sizeof(float)));
   glEnableVertexAttribArray(1);

   glBindVertexArray(VAO[1]);
   glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
   glBufferData(GL_ARRAY_BUFFER, sizeof(triangle2), triangle2, GL_STATIC_DRAW);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) 0);
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) (3 * sizeof(float)));
   glEnableVertexAttribArray(1);
}

void graphics_create_shaders()
{
   GLchar* vertex_shader_code = "#version 330 core\n"
      "layout (location = 0) in vec3 aPos;\n"
      "layout (location = 1) in vec3 aColor;\n"

      "out vec3 ourColor;\n"

      "void main()\n"
      "{\n"
      "   gl_Position = vec4(aPos, 1.0);\n"
      "   ourColor = aColor;\n"
      "}\0";

   GLchar* fragment_shader_code = "#version 330 core\n"
      "out vec4 fragColor;\n"
      "in vec3 ourColor;\n"

      "void main()\n"
      "{\n"
      "  fragColor = vec4(ourColor, 1.0);\n"
      "}\0";

   shader_program = glCreateProgram();
   if (shader_program == 0)
   {
      printf("Error creating shader program!\n");
      return;
   }

   add_shader(shader_program, vertex_shader_code, GL_VERTEX_SHADER);
   add_shader(shader_program, fragment_shader_code, GL_FRAGMENT_SHADER);

   glLinkProgram(shader_program);

   int sucess;
   char shader_prog_log[512];

   glGetProgramiv(shader_program, GL_LINK_STATUS, &sucess);
   if (sucess == 0) // shader program linking failure
   {
      glGetProgramInfoLog(shader_program, sizeof(shader_prog_log), NULL, shader_prog_log);
      printf("shader program linker error: %s\n", shader_prog_log);
      return;
   }
}

/**
 * create and compile a shader, also attaches the shader to a shader program
 * @param program program object id created from glCreateProgram
 * @param shader_code shader source code string
 * @param type type of shader to create, i.e: GL_VERTEX_SHADER
*/
static void add_shader(GLuint program, const GLchar* shader_code, GLenum type)
{
   GLuint shader = glCreateShader(type);
   if (shader == 0) // shader creation error
   {
      printf("shader creation error!\n");
      return;
   }

   glShaderSource(shader, 1, &shader_code, NULL);
   glCompileShader(shader);

   int sucess;
   char shader_log[512];

   glGetShaderiv(shader, GL_COMPILE_STATUS, &sucess);
   if (sucess == 0) // shader compile error
   {
      glGetShaderInfoLog(shader, sizeof(shader_log), NULL, shader_log);
      printf("Shader compilation error: %s", shader_log);
      return;
   }

   glAttachShader(program, shader);
}