#include <stdio.h>

#ifdef _WIN32
   #include <windows.h>
#elif __APPLE__

#endif

#include "SDL_syswm.h"

#include "../includes/display.h"
#include "../includes/util.h"

char* UTILS_open_file(const char* filter)
{
   SDL_SysWMinfo wmInfo;
   SDL_VERSION(&wmInfo.version);
   SDL_GetWindowWMInfo( display_get_window(), &wmInfo );

   #ifdef _WIN32
      OPENFILENAMEA ofn;
      char szFile[256] = {0};
      ZeroMemory(&ofn, sizeof(OPENFILENAME));
      ofn.lStructSize = sizeof(OPENFILENAME);
      ofn.hwndOwner = wmInfo.info.win.window;
      ofn.lpstrFile = szFile;
      ofn.nMaxFile = sizeof(szFile);
      ofn.lpstrFilter = filter;
      ofn.nFilterIndex = 1;
      ofn.lpstrInitialDir = "E:\\Coding_Stuff\\BudgetNES\\roms";
      ofn.lpstrTitle = "Load NES rom...";
      ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
      if (GetOpenFileNameA(&ofn))
      {
         return ofn.lpstrFile;
      }
   #elif __APPLE__

      
   #endif

   return " ";
}
