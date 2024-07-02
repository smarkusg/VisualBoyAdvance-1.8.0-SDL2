/*
** smarkusg: part of the code comes from mplayer 1.5 AOS
** License: GPL v2 or later
*/

#ifdef __AMIGAOS4__

#include <exec/types.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#define AMIGA_VERSION "VBA"
#define AMIGA_VERSION_SIGN "VisualBoyAdvance 1.8.0 (2.07.2024)"

int AmigaOS_Open(int argc, char *argv[]); // returns -1 if a problem
void AmigaOS_Close(void);
void AmigaOS_ParseArg(int argc, char *argv[], int *new_argc, char ***new_argv);
void VARARGS68K EasyRequester(CONST_STRPTR text, CONST_STRPTR button, ...);
void AmigaOS_ScreenTitle(SDL_Window * window, char *filename);
#endif //AOS4