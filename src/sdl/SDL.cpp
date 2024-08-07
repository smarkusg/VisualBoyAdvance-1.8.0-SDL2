// VisualBoyAdvance - Nintendo Gameboy/GameboyAdvance (TM) emulator.
// Copyright (C) 1999-2003 Forgotten
// Copyright (C) 2005-2006 Forgotten and the VBA development team

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or(at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <GL/gl.h>
#include <GL/glext.h>

#include "../AutoBuild.h"
//markus
#define AOS_SDL2 //SDL2_ON
#ifdef AOS_SDL2
#include "SDL2/SDL.h"
#include "logo.h"
#else
#include "SDL.h"
#endif
//
#include "../GBA.h"
#include "../agbprint.h"
#include "../Flash.h"
#include "../Port.h"
#include "debugger.h"
#include "../RTC.h"
#include "../Sound.h"
#include "../Text.h"
#include "../unzip.h"
#include "../Util.h"
#include "../gb/GB.h"
#include "../gb/gbGlobals.h"

#ifndef _WIN32
# include <unistd.h>
# define GETCWD getcwd
#else // _WIN32
# include <direct.h>
# define GETCWD _getcwd
#endif // _WIN32

#ifndef __GNUC__
# define HAVE_DECL_GETOPT 0
# define __STDC__ 1
# include "../getopt.h"
#else // ! __GNUC__
# define HAVE_DECL_GETOPT 1
# include "getopt.h"
#endif // ! __GNUC__

#ifdef MMX
extern "C" bool cpu_mmx;
#endif
extern bool soundEcho;
extern bool soundLowPass;
extern bool soundReverse;
extern int Init_2xSaI(u32);
extern void _2xSaI(u8*,u32,u8*,u8*,u32,int,int);
extern void _2xSaI32(u8*,u32,u8*,u8*,u32,int,int);  
extern void Super2xSaI(u8*,u32,u8*,u8*,u32,int,int);
extern void Super2xSaI32(u8*,u32,u8*,u8*,u32,int,int);
extern void SuperEagle(u8*,u32,u8*,u8*,u32,int,int);
extern void SuperEagle32(u8*,u32,u8*,u8*,u32,int,int);  
extern void Pixelate(u8*,u32,u8*,u8*,u32,int,int);
extern void Pixelate32(u8*,u32,u8*,u8*,u32,int,int);
extern void MotionBlur(u8*,u32,u8*,u8*,u32,int,int);
extern void MotionBlur32(u8*,u32,u8*,u8*,u32,int,int);
extern void AdMame2x(u8*,u32,u8*,u8*,u32,int,int);
extern void AdMame2x32(u8*,u32,u8*,u8*,u32,int,int);
extern void Simple2x(u8*,u32,u8*,u8*,u32,int,int);
extern void Simple2x32(u8*,u32,u8*,u8*,u32,int,int);
extern void Bilinear(u8*,u32,u8*,u8*,u32,int,int);
extern void Bilinear32(u8*,u32,u8*,u8*,u32,int,int);
extern void BilinearPlus(u8*,u32,u8*,u8*,u32,int,int);
extern void BilinearPlus32(u8*,u32,u8*,u8*,u32,int,int);
extern void Scanlines(u8*,u32,u8*,u8*,u32,int,int);
extern void Scanlines32(u8*,u32,u8*,u8*,u32,int,int);
extern void ScanlinesTV(u8*,u32,u8*,u8*,u32,int,int);
extern void ScanlinesTV32(u8*,u32,u8*,u8*,u32,int,int);
extern void hq2x(u8*,u32,u8*,u8*,u32,int,int);
extern void hq2x32(u8*,u32,u8*,u8*,u32,int,int);
extern void lq2x(u8*,u32,u8*,u8*,u32,int,int);
extern void lq2x32(u8*,u32,u8*,u8*,u32,int,int);

extern void SmartIB(u8*,u32,int,int);
extern void SmartIB32(u8*,u32,int,int);
extern void MotionBlurIB(u8*,u32,int,int);
extern void MotionBlurIB32(u8*,u32,int,int);

//markus
//void Init_Overlay(SDL_Surface *surface, int overlaytype);
//void Quit_Overlay(void);
//void Draw_Overlay(SDL_Surface *surface, int size);

extern void remoteInit();
extern void remoteCleanUp();
extern void remoteStubMain();
extern void remoteStubSignal(int,int);
extern void remoteOutput(char *, u32);
extern void remoteSetProtocol(int);
extern void remoteSetPort(int);
extern void debuggerOutput(char *, u32);

extern void CPUUpdateRenderBuffers(bool);
extern int gbHardware;

struct EmulatedSystem emulator = {
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  false,
  0
};
//markus
int debug_fprintf = 0;//simple stupid but it works

int flags = 0;
int mute = 0;

#ifdef AOS_SDL2
//new markus
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;
SDL_Surface *surface = NULL;
SDL_GLContext context = NULL;

int desktopWidth = 0;
int desktopHeight = 0;
char* dropped_file;
int droppwindow = 0;
#else
SDL_Surface *surface = NULL;
SDL_Overlay *overlay = NULL;
SDL_Rect overlay_rect;
#endif
//
int systemSpeed = 0;
int systemRedShift = 0;
int systemBlueShift = 0;
int systemGreenShift = 0;
int systemColorDepth = 0;
int systemDebug = 0;
int systemVerbose = 0;
int systemFrameSkip = 0;
int systemSaveUpdateCounter = SYSTEM_SAVE_NOT_UPDATED;

int srcPitch = 0;
int srcWidth = 0;
int srcHeight = 0;
int destWidth = 0;
int destHeight = 0;

int sensorX = 2047;
int sensorY = 2047;

int filter = 0;
u8 *delta = NULL;

int sdlPrintUsage = 0;
int disableMMX = 0;

int cartridgeType = 3;
int sizeOption = 0;
int captureFormat = 0;

int openGL = 0;
int textureSize = 256;
GLuint screenTexture = 0;
u8 *filterPix = 0;

int fullscreen_window = 0;

int pauseWhenInactive = 0;
int active = 1;
int emulating = 0;
int RGB_LOW_BITS_MASK=0x821;
u32 systemColorMap32[0x10000];
u16 systemColorMap16[0x10000];
u16 systemGbPalette[24];
void (*filterFunction)(u8*,u32,u8*,u8*,u32,int,int) = NULL;
void (*ifbFunction)(u8*,u32,int,int) = NULL;
int ifbType = 0;
char filename[2048];
char ipsname[2048];
char biosFileName[2048];
char captureDir[2048];
char saveDir[2048];
char batteryDir[2048];
#ifdef __AMIGAOS4__
char* homeDir = "/PROGDIR";
extern char* SDL_FULL; //tooltype from amigaos.cpp
#else
char* homeDir = NULL;
char* SDL_FULL = NULL;
#endif //AOS

/* Directory within homedir to use for default save location. */
#ifdef __AMIGAOS4__
#define DOT_DIR  "vba"
#define DOT_SAV  "saves"
#define DOT_RAM  "ram"
#define DOT_PIC  "screenshots"
#define DOT_BIOS "bios"

#else
#define DOT_DIR ".vba"
#endif

static char *rewindMemory = NULL;
static int rewindPos = 0;
static int rewindTopPos = 0;
static int rewindCounter = 0;
static int rewindCount = 0;
static bool rewindSaveNeeded = false;
static int rewindTimer = 0;

#define REWIND_SIZE 400000
#define SYSMSG_BUFFER_SIZE 1024

#ifdef __AMIGAOS4__
#include "amigaos.h"
#endif

#define _stricmp strcasecmp

bool sdlButtons[4][12] = {
  { false, false, false, false, false, false, 
    false, false, false, false, false, false },
  { false, false, false, false, false, false,
    false, false, false, false, false, false },
  { false, false, false, false, false, false,
    false, false, false, false, false, false },
  { false, false, false, false, false, false,
    false, false, false, false, false, false }
};

bool sdlMotionButtons[4] = { false, false, false, false };

int sdlNumDevices = 0;
SDL_Joystick **sdlDevices = NULL;

bool wasPaused = false;
int autoFrameSkip = 0;
int frameskipadjust = 0;
int showRenderedFrames = 0;
int renderedFrames = 0;

int throttle = 0;
u32 throttleLastTime = 0;
u32 autoFrameSkipLastTime = 0;

int showSpeed = 1;
int showSpeedTransparent = 1;
bool disableStatusMessages = false;
bool paused = false;
bool pauseNextFrame = false;
bool debugger = false;
bool debuggerStub = false;
int fullscreen = 0;
bool systemSoundOn = false;
bool yuv = false;
int yuvType = 0;
bool removeIntros = false;
int sdlFlashSize = 0;
int sdlAutoIPS = 1;
int sdlRtcEnable = 0;
int sdlAgbPrint = 0;
int sdlMirroringEnable = 0;

int sdlDefaultJoypad = 0;

extern void debuggerSignal(int,int);

void (*dbgMain)() = debuggerMain;
void (*dbgSignal)(int,int) = debuggerSignal;
void (*dbgOutput)(char *, u32) = debuggerOutput;

int  mouseCounter = 0;
int autoFire = 0;
bool autoFireToggle = false;

bool screenMessage = false;
char screenMessageBuffer[21];
u32  screenMessageTime = 0;

const  int        sdlSoundSamples  = 2048;
const  int        sdlSoundAlign    = 4;
const  int        sdlSoundCapacity = sdlSoundSamples * 2;
const  int        sdlSoundTotalLen = sdlSoundCapacity + sdlSoundAlign;
static u8         sdlSoundBuffer[sdlSoundTotalLen];
static int        sdlSoundRPos;
static int        sdlSoundWPos;
static SDL_cond  *sdlSoundCond;
static SDL_mutex *sdlSoundMutex;

static inline int soundBufferFree()
{
  int ret = sdlSoundRPos - sdlSoundWPos - sdlSoundAlign;
  if (ret < 0)
    ret += sdlSoundTotalLen;
  return ret;
}

static inline int soundBufferUsed()
{
  int ret = sdlSoundWPos - sdlSoundRPos;
  if (ret < 0)
    ret += sdlSoundTotalLen;
  return ret;
}

char *arg0;

#ifndef C_CORE
u8 sdlStretcher[16384];
int sdlStretcherPos;
#else
void (*sdlStretcher)(u8 *, u8*) = NULL;
#endif

enum {
  KEY_LEFT, KEY_RIGHT,
  KEY_UP, KEY_DOWN,
  KEY_BUTTON_A, KEY_BUTTON_B,
  KEY_BUTTON_START, KEY_BUTTON_SELECT,
  KEY_BUTTON_L, KEY_BUTTON_R,
  KEY_BUTTON_SPEED, KEY_BUTTON_CAPTURE
};
#ifdef AOS_SDL2
unsigned int joypad[4][12] = {
#else
u16 joypad[4][12] = { //markus
#endif
  { SDLK_LEFT,  SDLK_RIGHT,
    SDLK_UP,    SDLK_DOWN,
    SDLK_z,     SDLK_x,
    SDLK_RETURN,SDLK_BACKSPACE,
    SDLK_a,     SDLK_s,
    SDLK_SPACE, SDLK_F12
  },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

#ifdef AOS_SDL2
unsigned int defaultJoypad[12] = { //markus
#else
u16 defaultJoypad[12] = { //markus
#endif
  SDLK_LEFT,  SDLK_RIGHT,
  SDLK_UP,    SDLK_DOWN,
  SDLK_z,     SDLK_x,
  SDLK_RETURN,SDLK_BACKSPACE,
  SDLK_a,     SDLK_s,
  SDLK_SPACE, SDLK_F12
};
#ifdef AOS_SDL2
unsigned int motion[4] = {
SDLK_KP_4, SDLK_KP_6, SDLK_KP_8, SDLK_KP_2
#else
u16 motion[4] = {
  SDLK_KP4, SDLK_KP6, SDLK_KP8, SDLK_KP2
#endif
};

#ifdef AOS_SDL2
unsigned int defaultMotion[4] = {
  SDLK_KP_4, SDLK_KP_6, SDLK_KP_8, SDLK_KP_2
#else
u16 defaultMotion[4] = {
  SDLK_KP4, SDLK_KP6, SDLK_KP8, SDLK_KP2
#endif
};
struct option sdlOptions[] = {
  { "agb-print", no_argument, &sdlAgbPrint, 1 },
  { "auto-frameskip", no_argument, &autoFrameSkip, 1 },  
  { "bios", required_argument, 0, 'b' },
  { "config", required_argument, 0, 'c' },
  { "debug", no_argument, 0, 'd' },
  { "filter", required_argument, 0, 'f' },
  { "filter-normal", no_argument, &filter, 0 },
  { "filter-tv-mode", no_argument, &filter, 1 },
  { "filter-2xsai", no_argument, &filter, 2 },
  { "filter-super-2xsai", no_argument, &filter, 3 },
  { "filter-super-eagle", no_argument, &filter, 4 },
  { "filter-pixelate", no_argument, &filter, 5 },
  { "filter-motion-blur", no_argument, &filter, 6 },
  { "filter-advmame", no_argument, &filter, 7 },
  { "filter-simple2x", no_argument, &filter, 8 },
  { "filter-bilinear", no_argument, &filter, 9 },
  { "filter-bilinear+", no_argument, &filter, 10 },
  { "filter-scanlines", no_argument, &filter, 11 },
  { "filter-hq2x", no_argument, &filter, 12 },
  { "filter-lq2x", no_argument, &filter, 13 },
  { "flash-size", required_argument, 0, 'S' },
  { "flash-64k", no_argument, &sdlFlashSize, 0 },
  { "flash-128k", no_argument, &sdlFlashSize, 1 },
  { "frameskip", required_argument, 0, 's' },
  { "fullscreen", no_argument, &fullscreen, 1 },
  { "gdb", required_argument, 0, 'G' },
  { "help", no_argument, &sdlPrintUsage, 1 },
  { "ifb-none", no_argument, &ifbType, 0 },
  { "ifb-motion-blur", no_argument, &ifbType, 1 },
  { "ifb-smart", no_argument, &ifbType, 2 },
  { "ips", required_argument, 0, 'i' },
  { "no-agb-print", no_argument, &sdlAgbPrint, 0 },
  { "no-auto-frameskip", no_argument, &autoFrameSkip, 0 },
  { "no-debug", no_argument, 0, 'N' },
  { "no-ips", no_argument, &sdlAutoIPS, 0 },
#ifndef __AMIGAOS4__
  { "no-mmx", no_argument, &disableMMX, 1 },
#ifdef USE_OPENGL
  { "no-opengl", no_argument, &openGL, 0 },
#endif
#endif //AOS4
  { "no-pause-when-inactive", no_argument, &pauseWhenInactive, 0 },
#ifdef AOS_SDL2
  { "sdl2wfd", no_argument, &fullscreen_window, 1 },
#endif
  { "mute", no_argument, &mute, 1 },
  { "no-rtc", no_argument, &sdlRtcEnable, 0 },
  { "no-show-speed", no_argument, &showSpeed, 0 },
  { "no-throttle", no_argument, &throttle, 0 },
#ifndef __AMIGAOS4__
#ifdef USE_OPENGL
  { "opengl", required_argument, 0, 'O' },
  { "opengl-nearest", no_argument, &openGL, 1 },
  { "opengl-bilinear", no_argument, &openGL, 2 },
#endif
#endif //AOS4
  { "pause-when-inactive", no_argument, &pauseWhenInactive, 1 },
  { "profile", optional_argument, 0, 'p' },
  { "rtc", no_argument, &sdlRtcEnable, 1 },
  { "save-type", required_argument, 0, 't' },
  { "save-auto", no_argument, &cpuSaveType, 0 },
  { "save-eeprom", no_argument, &cpuSaveType, 1 },
  { "save-sram", no_argument, &cpuSaveType, 2 },
  { "save-flash", no_argument, &cpuSaveType, 3 },
  { "save-sensor", no_argument, &cpuSaveType, 4 },
  { "save-none", no_argument, &cpuSaveType, 5 },
  { "show-speed-normal", no_argument, &showSpeed, 1 },
  { "show-speed-detailed", no_argument, &showSpeed, 2 },
  { "throttle", required_argument, 0, 'T' },
  { "verbose", required_argument, 0, 'v' },  
  { "video-1x", no_argument, &sizeOption, 0 },
  { "video-2x", no_argument, &sizeOption, 1 },
  { "video-3x", no_argument, &sizeOption, 2 },
  { "video-4x", no_argument, &sizeOption, 3 },
  { NULL, no_argument, NULL, 0 }
};

extern bool CPUIsGBAImage(char *);
extern bool gbIsGameboyRom(char *);

#ifndef C_CORE
#define SDL_LONG(val) \
  *((u32 *)&sdlStretcher[sdlStretcherPos]) = val;\
  sdlStretcherPos+=4;

#define SDL_AND_EAX(val) \
  sdlStretcher[sdlStretcherPos++] = 0x25;\
  SDL_LONG(val);

#define SDL_AND_EBX(val) \
  sdlStretcher[sdlStretcherPos++] = 0x81;\
  sdlStretcher[sdlStretcherPos++] = 0xe3;\
  SDL_LONG(val);

#define SDL_OR_EAX_EBX \
  sdlStretcher[sdlStretcherPos++] = 0x09;\
  sdlStretcher[sdlStretcherPos++] = 0xd8;

#define SDL_LOADL_EBX \
  sdlStretcher[sdlStretcherPos++] = 0x8b;\
  sdlStretcher[sdlStretcherPos++] = 0x1f;

#define SDL_LOADW \
  sdlStretcher[sdlStretcherPos++] = 0x66;\
  sdlStretcher[sdlStretcherPos++] = 0x8b;\
  sdlStretcher[sdlStretcherPos++] = 0x06;\
  sdlStretcher[sdlStretcherPos++] = 0x83;\
  sdlStretcher[sdlStretcherPos++] = 0xc6;\
  sdlStretcher[sdlStretcherPos++] = 0x02;  

#define SDL_LOADL \
  sdlStretcher[sdlStretcherPos++] = 0x8b;\
  sdlStretcher[sdlStretcherPos++] = 0x06;\
  sdlStretcher[sdlStretcherPos++] = 0x83;\
  sdlStretcher[sdlStretcherPos++] = 0xc6;\
  sdlStretcher[sdlStretcherPos++] = 0x04;  

#define SDL_LOADL2 \
  sdlStretcher[sdlStretcherPos++] = 0x8b;\
  sdlStretcher[sdlStretcherPos++] = 0x06;\
  sdlStretcher[sdlStretcherPos++] = 0x83;\
  sdlStretcher[sdlStretcherPos++] = 0xc6;\
  sdlStretcher[sdlStretcherPos++] = 0x03;  

#define SDL_STOREW \
  sdlStretcher[sdlStretcherPos++] = 0x66;\
  sdlStretcher[sdlStretcherPos++] = 0x89;\
  sdlStretcher[sdlStretcherPos++] = 0x07;\
  sdlStretcher[sdlStretcherPos++] = 0x83;\
  sdlStretcher[sdlStretcherPos++] = 0xc7;\
  sdlStretcher[sdlStretcherPos++] = 0x02;  

#define SDL_STOREL \
  sdlStretcher[sdlStretcherPos++] = 0x89;\
  sdlStretcher[sdlStretcherPos++] = 0x07;\
  sdlStretcher[sdlStretcherPos++] = 0x83;\
  sdlStretcher[sdlStretcherPos++] = 0xc7;\
  sdlStretcher[sdlStretcherPos++] = 0x04;  

#define SDL_STOREL2 \
  sdlStretcher[sdlStretcherPos++] = 0x89;\
  sdlStretcher[sdlStretcherPos++] = 0x07;\
  sdlStretcher[sdlStretcherPos++] = 0x83;\
  sdlStretcher[sdlStretcherPos++] = 0xc7;\
  sdlStretcher[sdlStretcherPos++] = 0x03;  

#define SDL_RET \
  sdlStretcher[sdlStretcherPos++] = 0xc3;

#define SDL_PUSH_EAX \
  sdlStretcher[sdlStretcherPos++] = 0x50;

#define SDL_PUSH_ECX \
  sdlStretcher[sdlStretcherPos++] = 0x51;

#define SDL_PUSH_EBX \
  sdlStretcher[sdlStretcherPos++] = 0x53;

#define SDL_PUSH_ESI \
  sdlStretcher[sdlStretcherPos++] = 0x56;

#define SDL_PUSH_EDI \
  sdlStretcher[sdlStretcherPos++] = 0x57;

#define SDL_POP_EAX \
  sdlStretcher[sdlStretcherPos++] = 0x58;

#define SDL_POP_ECX \
  sdlStretcher[sdlStretcherPos++] = 0x59;

#define SDL_POP_EBX \
  sdlStretcher[sdlStretcherPos++] = 0x5b;

#define SDL_POP_ESI \
  sdlStretcher[sdlStretcherPos++] = 0x5e;

#define SDL_POP_EDI \
  sdlStretcher[sdlStretcherPos++] = 0x5f;

#define SDL_MOV_ECX(val) \
  sdlStretcher[sdlStretcherPos++] = 0xb9;\
  SDL_LONG(val);

#define SDL_REP_MOVSB \
  sdlStretcher[sdlStretcherPos++] = 0xf3;\
  sdlStretcher[sdlStretcherPos++] = 0xa4;

#define SDL_REP_MOVSW \
  sdlStretcher[sdlStretcherPos++] = 0xf3;\
  sdlStretcher[sdlStretcherPos++] = 0x66;\
  sdlStretcher[sdlStretcherPos++] = 0xa5;

#define SDL_REP_MOVSL \
  sdlStretcher[sdlStretcherPos++] = 0xf3;\
  sdlStretcher[sdlStretcherPos++] = 0xa5;

void sdlMakeStretcher(int width)
{
  sdlStretcherPos = 0;
  switch(systemColorDepth) {
  case 16:
    if(sizeOption) {
      SDL_PUSH_EAX;
      SDL_PUSH_ESI;
      SDL_PUSH_EDI;
      for(int i = 0; i < width; i++) {
        SDL_LOADW;
        SDL_STOREW;
        SDL_STOREW;
        if(sizeOption > 1) {
          SDL_STOREW;
        }
        if(sizeOption > 2) {
          SDL_STOREW;
        }
      }
      SDL_POP_EDI;
      SDL_POP_ESI;
      SDL_POP_EAX;
      SDL_RET;
    } else {
      SDL_PUSH_ESI;
      SDL_PUSH_EDI;
      SDL_PUSH_ECX;
      SDL_MOV_ECX(width);
      SDL_REP_MOVSW;
      SDL_POP_ECX;
      SDL_POP_EDI;
      SDL_POP_ESI;
      SDL_RET;
    }
    break;
  case 24:
    if(sizeOption) {
      SDL_PUSH_EAX;
      SDL_PUSH_ESI;
      SDL_PUSH_EDI;
      int w = width - 1;
      for(int i = 0; i < w; i++) {
        SDL_LOADL2;
        SDL_STOREL2;
        SDL_STOREL2;
        if(sizeOption > 1) {
          SDL_STOREL2;
        }
        if(sizeOption > 2) {
          SDL_STOREL2;
        }
      }
      // need to write the last one
      SDL_LOADL2;
      SDL_STOREL2;
      if(sizeOption > 1) {
        SDL_STOREL2;
      }
      if(sizeOption > 2) {
        SDL_STOREL2;
      }
      SDL_AND_EAX(0x00ffffff);
      SDL_PUSH_EBX;
      SDL_LOADL_EBX;
      SDL_AND_EBX(0xff000000);
      SDL_OR_EAX_EBX;
      SDL_POP_EBX;
      SDL_STOREL2;
      SDL_POP_EDI;
      SDL_POP_ESI;
      SDL_POP_EAX;
      SDL_RET;
    } else {
      SDL_PUSH_ESI;
      SDL_PUSH_EDI;
      SDL_PUSH_ECX;
      SDL_MOV_ECX(3*width);
      SDL_REP_MOVSB;
      SDL_POP_ECX;
      SDL_POP_EDI;
      SDL_POP_ESI;
      SDL_RET;
    }
    break;
  case 32:
    if(sizeOption) {
      SDL_PUSH_EAX;
      SDL_PUSH_ESI;
      SDL_PUSH_EDI;
      for(int i = 0; i < width; i++) {
        SDL_LOADL;
        SDL_STOREL;
        SDL_STOREL;
        if(sizeOption > 1) {
          SDL_STOREL;
        }
        if(sizeOption > 2) {
          SDL_STOREL;
        }
      }
      SDL_POP_EDI;
      SDL_POP_ESI;
      SDL_POP_EAX;
      SDL_RET;
    } else {
      SDL_PUSH_ESI;
      SDL_PUSH_EDI;
      SDL_PUSH_ECX;
      SDL_MOV_ECX(width);
      SDL_REP_MOVSL;
      SDL_POP_ECX;
      SDL_POP_EDI;
      SDL_POP_ESI;
      SDL_RET;
    }
    break;
  }
}

#ifdef _MSC_VER
#define SDL_CALL_STRETCHER \
  {\
    __asm mov eax, stretcher\
    __asm mov edi, dest\
    __asm mov esi, src\
    __asm call eax\
  }
#else
#define SDL_CALL_STRETCHER \
        asm volatile("call *%%eax"::"a" (stretcher),"S" (src),"D" (dest))
#endif
#else
#define SDL_CALL_STRETCHER \
       sdlStretcher(src, dest)

void sdlStretch16x1(u8 *src, u8 *dest)
{
  u16 *s = (u16 *)src;
  u16 *d = (u16 *)dest;
  for(int i = 0; i < srcWidth; i++)
    *d++ = *s++;
}

void sdlStretch16x2(u8 *src, u8 *dest)
{
  u16 *s = (u16 *)src;
  u16 *d = (u16 *)dest;
  for(int i = 0; i < srcWidth; i++) {
    *d++ = *s;
    *d++ = *s++;
  }
}

void sdlStretch16x3(u8 *src, u8 *dest)
{
  u16 *s = (u16 *)src;
  u16 *d = (u16 *)dest;
  for(int i = 0; i < srcWidth; i++) {
    *d++ = *s;
    *d++ = *s;
    *d++ = *s++;
  }
}

void sdlStretch16x4(u8 *src, u8 *dest)
{
  u16 *s = (u16 *)src;
  u16 *d = (u16 *)dest;
  for(int i = 0; i < srcWidth; i++) {
    *d++ = *s;
    *d++ = *s;
    *d++ = *s;
    *d++ = *s++;
  }
}

void (*sdlStretcher16[4])(u8 *, u8 *) = {
  sdlStretch16x1,
  sdlStretch16x2,
  sdlStretch16x3,
  sdlStretch16x4
};

void sdlStretch32x1(u8 *src, u8 *dest)
{
  u32 *s = (u32 *)src;
  u32 *d = (u32 *)dest;
  for(int i = 0; i < srcWidth; i++)
    *d++ = *s++;
}

void sdlStretch32x2(u8 *src, u8 *dest)
{
  u32 *s = (u32 *)src;
  u32 *d = (u32 *)dest;
  for(int i = 0; i < srcWidth; i++) {
    *d++ = *s;
    *d++ = *s++;
  }
}

void sdlStretch32x3(u8 *src, u8 *dest)
{
  u32 *s = (u32 *)src;
  u32 *d = (u32 *)dest;
  for(int i = 0; i < srcWidth; i++) {
    *d++ = *s;
    *d++ = *s;
    *d++ = *s++;
  }
}

void sdlStretch32x4(u8 *src, u8 *dest)
{
  u32 *s = (u32 *)src;
  u32 *d = (u32 *)dest;
  for(int i = 0; i < srcWidth; i++) {
    *d++ = *s;
    *d++ = *s;
    *d++ = *s;
    *d++ = *s++;
  }
}

void (*sdlStretcher32[4])(u8 *, u8 *) = {
  sdlStretch32x1,
  sdlStretch32x2,
  sdlStretch32x3,
  sdlStretch32x4
};

void sdlStretch24x1(u8 *src, u8 *dest)
{
  u8 *s = src;
  u8 *d = dest;
  for(int i = 0; i < srcWidth; i++) {
    *d++ = *s++;
    *d++ = *s++;
    *d++ = *s++;
  }
}

void sdlStretch24x2(u8 *src, u8 *dest)
{
  u8 *s = (u8 *)src;
  u8 *d = (u8 *)dest;
  for(int i = 0; i < srcWidth; i++) {
    *d++ = *s;
    *d++ = *(s+1);
    *d++ = *(s+2);
    s += 3;
    *d++ = *s;
    *d++ = *(s+1);
    *d++ = *(s+2);
    s += 3;
  }
}

void sdlStretch24x3(u8 *src, u8 *dest)
{
  u8 *s = (u8 *)src;
  u8 *d = (u8 *)dest;
  for(int i = 0; i < srcWidth; i++) {
    *d++ = *s;
    *d++ = *(s+1);
    *d++ = *(s+2);
    s += 3;
    *d++ = *s;
    *d++ = *(s+1);
    *d++ = *(s+2);
    s += 3;
    *d++ = *s;
    *d++ = *(s+1);
    *d++ = *(s+2);
    s += 3;
  }
}

void sdlStretch24x4(u8 *src, u8 *dest)
{
  u8 *s = (u8 *)src;
  u8 *d = (u8 *)dest;
  for(int i = 0; i < srcWidth; i++) {
    *d++ = *s;
    *d++ = *(s+1);
    *d++ = *(s+2);
    s += 3;
    *d++ = *s;
    *d++ = *(s+1);
    *d++ = *(s+2);
    s += 3;
    *d++ = *s;
    *d++ = *(s+1);
    *d++ = *(s+2);
    s += 3;
    *d++ = *s;
    *d++ = *(s+1);
    *d++ = *(s+2);
    s += 3;
  }
}

void (*sdlStretcher24[4])(u8 *, u8 *) = {
  sdlStretch24x1,
  sdlStretch24x2,
  sdlStretch24x3,
  sdlStretch24x4
};

#endif

u32 sdlFromHex(char *s)
{
  u32 value;
  sscanf(s, "%x", &value);
  return value;
}

#ifdef __MSC__
#define stat _stat
#define S_IFDIR _S_IFDIR
#endif

void sdlCheckDirectory(char *dir)
{
  struct stat buf;

  int len = strlen(dir);

  char *p = dir + len - 1;

  if(*p == '/' ||
     *p == '\\')
    *p = 0;
  
  if(stat(dir, &buf) == 0) {
    if(!(buf.st_mode & S_IFDIR)) {
      if (debug_fprintf) fprintf(stderr, "Error: %s is not a directory\n", dir);
      dir[0] = 0;
    }
  } else {
    if (debug_fprintf) fprintf(stderr, "Error: %s does not exist\n", dir);
    dir[0] = 0;
  }
}


char *sdlGetFilename(char *name)
{
  static char filebuffer[2048];

  int len = strlen(name);
  
  char *p = name + len - 1;
  
  while(true) {
    if(*p == '/' ||
#ifdef __AMIGAOS4__
       *p == ':' ||
#endif
       *p == '\\') {
      p++;
      break;
    }
    len--;
    p--;
    if(len == 0)
      break;
  }
  
  if(len == 0)
    strcpy(filebuffer, name);
  else
    strcpy(filebuffer, p);
  return filebuffer;
}

FILE *sdlFindFile(const char *name)
{
  char buffer[4096];
  char path[2048];

#ifdef _WIN32
#define PATH_SEP ";"
#define FILE_SEP '\\'
#define EXE_NAME "VisualBoyAdvance-SDL.exe"
#else // ! _WIN32
#define PATH_SEP ":"
#define FILE_SEP '/'
#define EXE_NAME "VisualBoyAdvance"
#endif // ! _WIN32

  if (debug_fprintf) fprintf(stderr, "Searching for file %s\n", name);
  
  if(GETCWD(buffer, 2048)) {
    if (debug_fprintf) fprintf(stderr, "Searching current directory: %s\n", buffer);
  }
  
  FILE *f = fopen(name, "r");
  if(f != NULL) {
    return f;
  }

  if(homeDir) {
    if (debug_fprintf) fprintf(stderr, "Searching home directory: %s%c%s\n", homeDir, FILE_SEP,
        DOT_DIR);
    sprintf(path, "%s%c%s%c%s", homeDir, FILE_SEP, DOT_DIR, FILE_SEP, name);
    f = fopen(path, "r");
    if(f != NULL)
      return f;
  }

#ifdef _WIN32
  home = getenv("USERPROFILE");
  if(home != NULL) {
    if (debug_fprintf) fprintf(stderr, "Searching user profile directory: %s\n", home);
    sprintf(path, "%s%c%s", home, FILE_SEP, name);
    f = fopen(path, "r");
    if(f != NULL)
      return f;
  }
#else // ! _WIN32
#ifdef __AMIGAOS4__
    if (debug_fprintf) fprintf(stderr, "Searching system config directory: %s\n", "/PROGDIR/vba");
    sprintf(path, "%s%c%s", "PROGDIR:vba", FILE_SEP, name);
#else
    if (debug_fprintf) fprintf(stderr, "Searching system config directory: %s\n", SYSCONFDIR);
    sprintf(path, "%s%c%s", SYSCONFDIR, FILE_SEP, name);
#endif //AOS
    f = fopen(path, "r");
    if(f != NULL)
      return f;
#endif // ! _WIN32

  if(!strchr(arg0, '/') &&
     !strchr(arg0, '\\')) {
    char *path = getenv("PATH");

    if(path != NULL) {
      if (debug_fprintf) fprintf(stderr, "Searching PATH\n");
      strncpy(buffer, path, 4096);
      buffer[4095] = 0;
      char *tok = strtok(buffer, PATH_SEP);
      
      while(tok) {
        sprintf(path, "%s%c%s", tok, FILE_SEP, EXE_NAME);
        f = fopen(path, "r");
        if(f != NULL) {
          char path2[2048];
          fclose(f);
          sprintf(path2, "%s%c%s", tok, FILE_SEP, name);
          f = fopen(path2, "r");
          if(f != NULL) {
            if (debug_fprintf) fprintf(stderr, "Found at %s\n", path2);
            return f;
          }
        }
        tok = strtok(NULL, PATH_SEP);
      }
    }
  } else {
    // executable is relative to some directory
    if (debug_fprintf) fprintf(stderr, "Searching executable directory\n");
    strcpy(buffer, arg0);
    char *p = strrchr(buffer, FILE_SEP);
    if(p) {
      *p = 0;
      sprintf(path, "%s%c%s", buffer, FILE_SEP, name);
      f = fopen(path, "r");
      if(f != NULL)
        return f;
    }
  }
  return NULL;
}

void sdlOpenGLInit(int w, int h)
{
  float screenAspect = (float) srcWidth / srcHeight,
        windowAspect = (float) w / h;

  if(glIsTexture(screenTexture))
  glDeleteTextures(1, &screenTexture);

  glDisable(GL_CULL_FACE);
  glEnable(GL_TEXTURE_2D);

  if(windowAspect == screenAspect)
    glViewport(0, 0, w, h);
  else if (windowAspect < screenAspect) {
    int height = (int)(w / screenAspect);
    glViewport(0, (h - height) / 2, w, height);
  } else {
    int width = (int)(h * screenAspect);
    glViewport((w - width) / 2, 0, width, h);
  }

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  glOrtho(0.0, 1.0, 1.0, 0.0, 0.0, 1.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glGenTextures(1, &screenTexture);
  glBindTexture(GL_TEXTURE_2D, screenTexture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                  openGL == 2 ? GL_LINEAR : GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  openGL == 2 ? GL_LINEAR : GL_NEAREST);

  textureSize = filterFunction ? 512 : 256;
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureSize, textureSize, 0,
               GL_BGRA, GL_UNSIGNED_BYTE, NULL);

//  glClearColor(0.0,0.0,0.0,1.0);
  glClear( GL_COLOR_BUFFER_BIT );
}

void sdlReadPreferences(FILE *f)
{
  char buffer[2048];
  
  while(1) {
    char *s = fgets(buffer, 2048, f);

    if(s == NULL)
      break;

    char *p  = strchr(s, '#');
    
    if(p)
      *p = 0;
    
    char *token = strtok(s, " \t\n\r=");

    if(!token)
      continue;

    if(strlen(token) == 0)
      continue;

    char *key = token;
    char *value = strtok(NULL, "\t\n\r");

    if(value == NULL) {
      if (debug_fprintf) fprintf(stderr, "Empty value for key %s\n", key);
      continue;
    }

    if(!strcmp(key,"Joy0_Left")) {
      joypad[0][KEY_LEFT] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy0_Right")) {
      joypad[0][KEY_RIGHT] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy0_Up")) {
      joypad[0][KEY_UP] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy0_Down")) {
      joypad[0][KEY_DOWN] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy0_A")) {
      joypad[0][KEY_BUTTON_A] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy0_B")) {
      joypad[0][KEY_BUTTON_B] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy0_L")) {
      joypad[0][KEY_BUTTON_L] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy0_R")) {
      joypad[0][KEY_BUTTON_R] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy0_Start")) {
      joypad[0][KEY_BUTTON_START] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy0_Select")) {
      joypad[0][KEY_BUTTON_SELECT] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy0_Speed")) {
      joypad[0][KEY_BUTTON_SPEED] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy0_Capture")) {
      joypad[0][KEY_BUTTON_CAPTURE] = sdlFromHex(value);
    } else if(!strcmp(key,"Joy1_Left")) {
      joypad[1][KEY_LEFT] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy1_Right")) {
      joypad[1][KEY_RIGHT] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy1_Up")) {
      joypad[1][KEY_UP] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy1_Down")) {
      joypad[1][KEY_DOWN] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy1_A")) {
      joypad[1][KEY_BUTTON_A] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy1_B")) {
      joypad[1][KEY_BUTTON_B] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy1_L")) {
      joypad[1][KEY_BUTTON_L] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy1_R")) {
      joypad[1][KEY_BUTTON_R] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy1_Start")) {
      joypad[1][KEY_BUTTON_START] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy1_Select")) {
      joypad[1][KEY_BUTTON_SELECT] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy1_Speed")) {
      joypad[1][KEY_BUTTON_SPEED] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy1_Capture")) {
      joypad[1][KEY_BUTTON_CAPTURE] = sdlFromHex(value);
    } else if(!strcmp(key,"Joy2_Left")) {
      joypad[2][KEY_LEFT] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy2_Right")) {
      joypad[2][KEY_RIGHT] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy2_Up")) {
      joypad[2][KEY_UP] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy2_Down")) {
      joypad[2][KEY_DOWN] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy2_A")) {
      joypad[2][KEY_BUTTON_A] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy2_B")) {
      joypad[2][KEY_BUTTON_B] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy2_L")) {
      joypad[2][KEY_BUTTON_L] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy2_R")) {
      joypad[2][KEY_BUTTON_R] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy2_Start")) {
      joypad[2][KEY_BUTTON_START] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy2_Select")) {
      joypad[2][KEY_BUTTON_SELECT] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy2_Speed")) {
      joypad[2][KEY_BUTTON_SPEED] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy2_Capture")) {
      joypad[2][KEY_BUTTON_CAPTURE] = sdlFromHex(value);
    } else if(!strcmp(key,"Joy4_Left")) {
      joypad[4][KEY_LEFT] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy4_Right")) {
      joypad[4][KEY_RIGHT] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy4_Up")) {
      joypad[4][KEY_UP] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy4_Down")) {
      joypad[4][KEY_DOWN] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy4_A")) {
      joypad[4][KEY_BUTTON_A] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy4_B")) {
      joypad[4][KEY_BUTTON_B] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy4_L")) {
      joypad[4][KEY_BUTTON_L] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy4_R")) {
      joypad[4][KEY_BUTTON_R] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy4_Start")) {
      joypad[4][KEY_BUTTON_START] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy4_Select")) {
      joypad[4][KEY_BUTTON_SELECT] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy4_Speed")) {
      joypad[4][KEY_BUTTON_SPEED] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy4_Capture")) {
      joypad[4][KEY_BUTTON_CAPTURE] = sdlFromHex(value);
    } else if(!strcmp(key, "Motion_Left")) {
      motion[KEY_LEFT] = sdlFromHex(value);
    } else if(!strcmp(key, "Motion_Right")) {
      motion[KEY_RIGHT] = sdlFromHex(value);
    } else if(!strcmp(key, "Motion_Up")) {
      motion[KEY_UP] = sdlFromHex(value);
    } else if(!strcmp(key, "Motion_Down")) {
      motion[KEY_DOWN] = sdlFromHex(value);
    } else if(!strcmp(key, "frameSkip")) {
      frameSkip = sdlFromHex(value);
      if(frameSkip < 0 || frameSkip > 9)
        frameSkip = 2;
    } else if(!strcmp(key, "gbFrameSkip")) {
      gbFrameSkip = sdlFromHex(value);
      if(gbFrameSkip < 0 || gbFrameSkip > 9)
        gbFrameSkip = 0;      
    } else if(!strcmp(key, "video")) {
      sizeOption = sdlFromHex(value);
      if(sizeOption < 0 || sizeOption > 3)
        sizeOption = 1;
    } else if(!strcmp(key, "fullScreen")) {
      fullscreen = sdlFromHex(value) ? 1 : 0;
#ifndef __AMIGAOS4__
    } else if(!strcmp(key, "openGL")) {
     openGL = sdlFromHex(value);
#endif
    } else if(!strcmp(key, "useBios")) {
      useBios = sdlFromHex(value) ? true : false;
    } else if(!strcmp(key, "skipBios")) {
      skipBios = sdlFromHex(value) ? true : false;
    } else if(!strcmp(key, "biosFile")) {
      strcpy(biosFileName, value);
    } else if(!strcmp(key, "filter")) {
      filter = sdlFromHex(value);
      if(filter < 0 || filter > 13)
        filter = 0;
    } else if(!strcmp(key, "disableStatus")) {
      disableStatusMessages = sdlFromHex(value) ? true : false;
    } else if(!strcmp(key, "borderOn")) {
      gbBorderOn = sdlFromHex(value) ? true : false;
    } else if(!strcmp(key, "borderAutomatic")) {
      gbBorderAutomatic = sdlFromHex(value) ? true : false;
    } else if(!strcmp(key, "emulatorType")) {
      gbEmulatorType = sdlFromHex(value);
      if(gbEmulatorType < 0 || gbEmulatorType > 5)
        gbEmulatorType = 1;
    } else if(!strcmp(key, "colorOption")) {
      gbColorOption = sdlFromHex(value) ? true : false;
    } else if(!strcmp(key, "captureDir")) {
      sdlCheckDirectory(value);
      strcpy(captureDir, value);
    } else if(!strcmp(key, "saveDir")) {
      sdlCheckDirectory(value);
      strcpy(saveDir, value);
    } else if(!strcmp(key, "batteryDir")) {
      sdlCheckDirectory(value);
      strcpy(batteryDir, value);
    } else if(!strcmp(key, "captureFormat")) {
      captureFormat = sdlFromHex(value);
    } else if(!strcmp(key, "soundQuality")) {
      soundQuality = sdlFromHex(value);
      switch(soundQuality) {
      case 1:
      case 2:
      case 4:
        break;
      default:
        if (debug_fprintf) fprintf(stderr, "Unknown sound quality %d. Defaulting to 22Khz\n", 
                soundQuality);
        soundQuality = 2;
        break;
      }
    } else if(!strcmp(key, "soundOff")) {
      soundOffFlag = sdlFromHex(value) ? true : false;
//dublicate!!
    } else if(!strcmp(key, "mute")) {
      mute  = sdlFromHex(value);
    } else if(!strcmp(key, "soundEnable")) {
      int res = sdlFromHex(value) & 0x30f;
      soundEnable(res);
      soundDisable(~res);
    } else if(!strcmp(key, "soundEcho")) {
      soundEcho = sdlFromHex(value) ? true : false;
    } else if(!strcmp(key, "soundLowPass")) {
      soundLowPass = sdlFromHex(value) ? true : false;
    } else if(!strcmp(key, "soundReverse")) {
      soundReverse = sdlFromHex(value) ? true : false;
    } else if(!strcmp(key, "soundVolume")) {
      soundVolume = sdlFromHex(value);
      if(soundVolume < 0 || soundVolume > 3)
        soundVolume = 0;
    } else if(!strcmp(key, "removeIntros")) {
      removeIntros = sdlFromHex(value) ? true : false;
    } else if(!strcmp(key, "saveType")) {
      cpuSaveType = sdlFromHex(value);
      if(cpuSaveType < 0 || cpuSaveType > 5)
        cpuSaveType = 0;
    } else if(!strcmp(key, "flashSize")) {
      sdlFlashSize = sdlFromHex(value);
      if(sdlFlashSize != 0 && sdlFlashSize != 1)
        sdlFlashSize = 0;
    } else if(!strcmp(key, "ifbType")) {
      ifbType = sdlFromHex(value);
      if(ifbType < 0 || ifbType > 2)
        ifbType = 0;
    } else if(!strcmp(key, "showSpeed")) {
      showSpeed = sdlFromHex(value);
      if(showSpeed < 0 || showSpeed > 2)
        showSpeed = 1;
    } else if(!strcmp(key, "showSpeedTransparent")) {
      showSpeedTransparent = sdlFromHex(value);
    } else if(!strcmp(key, "autoFrameSkip")) {
      autoFrameSkip = sdlFromHex(value);
    } else if(!strcmp(key, "throttle")) {
      throttle = sdlFromHex(value);
      if(throttle != 0 && (throttle < 5 || throttle > 1000))
        throttle = 0;
    } else if(!strcmp(key, "disableMMX")) {
#ifdef MMX
      cpu_mmx = sdlFromHex(value) ? false : true;
#endif
#ifdef AOS_SDL2
    } else if(!strcmp(key, "sdl2wfd")) {
      fullscreen_window = sdlFromHex(value) ? true : false;
    } else if(!strcmp(key, "droppwindow")) {
      droppwindow = sdlFromHex(value) ? true : false;
#endif
    } else if(!strcmp(key, "pauseWhenInactive")) {
      pauseWhenInactive = sdlFromHex(value) ? true : false;
    } else if(!strcmp(key, "agbPrint")) {
      sdlAgbPrint = sdlFromHex(value);
    } else if(!strcmp(key, "rtcEnabled")) {
      sdlRtcEnable = sdlFromHex(value);
    } else if(!strcmp(key, "rewindTimer")) {
      rewindTimer = sdlFromHex(value);
      if(rewindTimer < 0 || rewindTimer > 600)
        rewindTimer = 0;
      rewindTimer *= 6;  // convert value to 10 frames multiple
    } else {
      if (debug_fprintf) fprintf(stderr, "Unknown configuration key %s\n", key);
    }
  }
}

void sdlReadPreferences()
{
  FILE *f = sdlFindFile("VisualBoyAdvance.cfg");

  if(f == NULL) {
    if (debug_fprintf) fprintf(stderr, "Configuration file NOT FOUND (using defaults)\n");
    return;
  } else
    if (debug_fprintf) fprintf(stderr, "Reading configuration file.\n");

  sdlReadPreferences(f);

  fclose(f);
}

static void sdlApplyPerImagePreferences()
{
  FILE *f = sdlFindFile("vba-over.ini");
  if(!f) {
    if (debug_fprintf) fprintf(stderr, "vba-over.ini NOT FOUND (using emulator settings)\n");
    return;
  } else
    if (debug_fprintf) fprintf(stderr, "Reading vba-over.ini\n");

  char buffer[7];
  buffer[0] = '[';
  buffer[1] = rom[0xac];
  buffer[2] = rom[0xad];
  buffer[3] = rom[0xae];
  buffer[4] = rom[0xaf];
  buffer[5] = ']';
  buffer[6] = 0;

  char readBuffer[2048];

  bool found = false;
  
  while(1) {
    char *s = fgets(readBuffer, 2048, f);

    if(s == NULL)
      break;

    char *p  = strchr(s, ';');
    
    if(p)
      *p = 0;
    
    char *token = strtok(s, " \t\n\r=");

    if(!token)
      continue;
    if(strlen(token) == 0)
      continue;

    if(!strcmp(token, buffer)) {
      found = true;
      break;
    }
  }

  if(found) {
    while(1) {
      char *s = fgets(readBuffer, 2048, f);

      if(s == NULL)
        break;

      char *p = strchr(s, ';');
      if(p)
        *p = 0;

      char *token = strtok(s, " \t\n\r=");
      if(!token)
        continue;
      if(strlen(token) == 0)
        continue;

      if(token[0] == '[') // starting another image settings
        break;
      char *value = strtok(NULL, "\t\n\r=");
      if(value == NULL)
        continue;
      
      if(!strcmp(token, "rtcEnabled"))
        rtcEnable(atoi(value) == 0 ? false : true);
      else if(!strcmp(token, "flashSize")) {
        int size = atoi(value);
        if(size == 0x10000 || size == 0x20000)
          flashSetSize(size);
      } else if(!strcmp(token, "saveType")) {
        int save = atoi(value);
        if(save >= 0 && save <= 5)
          cpuSaveType = save;
      } else if(!strcmp(token, "mirroringEnabled")) {
        mirroringEnable = (atoi(value) == 0 ? false : true);
      }
    }
  }
  fclose(f);
}

static int sdlCalculateShift(u32 mask)
{
  int m = 0;
  
  while(mask) {
    m++;
    mask >>= 1;
  }

  return m-5;
}

static int sdlCalculateMaskWidth(u32 mask)
{
  int m = 0;
  int mask2 = mask;

  while(mask2) {
    m++;
    mask2 >>= 1;
  }

  int m2 = 0;
  mask2 = mask;
  while(!(mask2 & 1)) {
    m2++;
    mask2 >>= 1;
  }

  return m - m2;
}

#ifdef AOS_SDL2
//opengl sdl2 stuff
void sdlReadDesktopVideoMode()
{
    SDL_DisplayMode dm;
    SDL_GetDesktopDisplayMode(SDL_GetWindowDisplayIndex(window), &dm);
    desktopWidth = dm.w;
    desktopHeight = dm.h;
}

static void sdlOpenGLScaleWithAspect(int w, int h)
{
    glViewport(0, 0, w, h);
}
#endif
void sdlWriteState(int num)
{
  char stateName[2048];

  if(saveDir[0])
    sprintf(stateName, "%s/%s%d.sgm", saveDir, sdlGetFilename(filename),
            num+1);
  else if (homeDir)
#ifdef __AMIGAOS4__
    sprintf(stateName, "%s/%s/%s/%s%d.sgm", homeDir, DOT_DIR, DOT_SAV, sdlGetFilename(filename), num + 1);
#else
    sprintf(stateName, "%s/%s/%s%d.sgm", homeDir, DOT_DIR, sdlGetFilename(filename), num + 1);
#endif
  else
    sprintf(stateName,"%s%d.sgm", filename, num+1);
  
  if(emulator.emuWriteState)
    emulator.emuWriteState(stateName);

  sprintf(stateName, "Wrote state %d", num+1);
  systemScreenMessage(stateName);

  systemDrawScreen();
}

void sdlReadState(int num)
{
  char stateName[2048];

  if(saveDir[0])
    sprintf(stateName, "%s/%s%d.sgm", saveDir, sdlGetFilename(filename),
            num+1);
  else if (homeDir)
#ifdef __AMIGAOS4__
    sprintf(stateName, "%s/%s/%s/%s%d.sgm", homeDir, DOT_DIR, DOT_SAV, sdlGetFilename(filename), num + 1);
#else
    sprintf(stateName, "%s/%s/%s%d.sgm", homeDir, DOT_DIR, sdlGetFilename(filename), num + 1);
#endif
  else
    sprintf(stateName,"%s%d.sgm", filename, num+1);

  if(emulator.emuReadState)
    emulator.emuReadState(stateName);

  sprintf(stateName, "Loaded state %d", num+1);
  systemScreenMessage(stateName);

  systemDrawScreen();
}

void sdlWriteBattery()
{
  char buffer[1048];

  if(batteryDir[0])
    sprintf(buffer, "%s/%s.sav", batteryDir, sdlGetFilename(filename));
  else if (homeDir)
#ifdef __AMIGAOS4__
    sprintf(buffer, "%s/%s/%s/%s.sav", homeDir, DOT_DIR,  DOT_RAM, sdlGetFilename(filename));
#else
    sprintf(buffer, "%s/%s/%s.sav", homeDir, DOT_DIR, sdlGetFilename(filename));
#endif
  else  
    sprintf(buffer, "%s.sav", filename);

  emulator.emuWriteBattery(buffer);

#ifndef __AMIGAOS4__
  systemScreenMessage("Wrote battery");
#endif
}

void sdlReadBattery()
{
  char buffer[1048];
  
  if(batteryDir[0])
    sprintf(buffer, "%s/%s.sav", batteryDir, sdlGetFilename(filename));
  else if (homeDir)
#ifdef __AMIGAOS4__
    sprintf(buffer, "%s/%s/%s/%s.sav", homeDir, DOT_DIR, DOT_RAM, sdlGetFilename(filename));
#else
    sprintf(buffer, "%s/%s/%s.sav", homeDir, DOT_DIR, sdlGetFilename(filename));
#endif
  else 
    sprintf(buffer, "%s.sav", filename);
  
  bool res = false;

  res = emulator.emuReadBattery(buffer);

#ifndef __AMIGAOS4__
  if(res)
    systemScreenMessage("Loaded battery");
#endif
}

#define MOD_KEYS    (KMOD_CTRL|KMOD_SHIFT|KMOD_ALT)
#define MOD_NOCTRL  (KMOD_SHIFT|KMOD_ALT)
#define MOD_NOALT   (KMOD_CTRL|KMOD_SHIFT)
#define MOD_NOSHIFT (KMOD_CTRL|KMOD_ALT)

void sdlUpdateKey(int key, bool down)
{
  int i;
  for(int j = 0; j < 4; j++) {
    for(i = 0 ; i < 12; i++) {
#ifdef AOS_SDL2 
      if((joypad[j][i] & 0xf000) == 0) {
#endif
        if(key == joypad[j][i])
          sdlButtons[j][i] = down;
#ifdef AOS_SDL2 
      }
#endif
    }
  }
  for(i = 0 ; i < 4; i++) {
#ifdef AOS_SDL2 
    if((motion[i] & 0xf000) == 0) {
#endif
      if(key == motion[i])
        sdlMotionButtons[i] = down;
#ifdef AOS_SDL2 
    }
#endif
  }
}

void sdlUpdateJoyButton(int which,
                        int button,
                        bool pressed)
{
  int i;
  for(int j = 0; j < 4; j++) {
    for(i = 0; i < 12; i++) {
      int dev = (joypad[j][i] >> 12);
      int b = joypad[j][i] & 0xfff;
      if(dev) {
        dev--;
        
        if((dev == which) && (b >= 128) && (b == (button+128))) {
          sdlButtons[j][i] = pressed;
        }
      }
    }
  }
  for(i = 0; i < 4; i++) {
    int dev = (motion[i] >> 12);
    int b = motion[i] & 0xfff;
    if(dev) {
      dev--;

      if((dev == which) && (b >= 128) && (b == (button+128))) {
        sdlMotionButtons[i] = pressed;
      }
    }
  }  
}

void sdlUpdateJoyHat(int which,
                     int hat,
                     int value)
{
  int i;
  for(int j = 0; j < 4; j++) {
    for(i = 0; i < 12; i++) {
      int dev = (joypad[j][i] >> 12);
      int a = joypad[j][i] & 0xfff;
      if(dev) {
        dev--;
        
        if((dev == which) && (a>=32) && (a < 48) && (((a&15)>>2) == hat)) {
          int dir = a & 3;
          int v = 0;
          switch(dir) {
          case 0:
            v = value & SDL_HAT_UP;
            break;
          case 1:
            v = value & SDL_HAT_DOWN;
            break;
          case 2:
            v = value & SDL_HAT_RIGHT;
            break;
          case 3:
            v = value & SDL_HAT_LEFT;
            break;
          }
          sdlButtons[j][i] = (v ? true : false);
        }
      }
    }
  }
  for(i = 0; i < 4; i++) {
    int dev = (motion[i] >> 12);
    int a = motion[i] & 0xfff;
    if(dev) {
      dev--;

      if((dev == which) && (a>=32) && (a < 48) && (((a&15)>>2) == hat)) {
        int dir = a & 3;
        int v = 0;
        switch(dir) {
        case 0:
          v = value & SDL_HAT_UP;
          break;
        case 1:
          v = value & SDL_HAT_DOWN;
          break;
        case 2:
          v = value & SDL_HAT_RIGHT;
          break;
        case 3:
          v = value & SDL_HAT_LEFT;
          break;
        }
        sdlMotionButtons[i] = (v ? true : false);
      }
    }
  }      
}

void sdlUpdateJoyAxis(int which,
                      int axis,
                      int value)
{
  int i;
  for(int j = 0; j < 4; j++) {
    for(i = 0; i < 12; i++) {
      int dev = (joypad[j][i] >> 12);
      int a = joypad[j][i] & 0xfff;
      if(dev) {
        dev--;
        
        if((dev == which) && (a < 32) && ((a>>1) == axis)) {
          sdlButtons[j][i] = (a & 1) ? (value > 16384) : (value < -16384);
        }
      }
    }
  }
  for(i = 0; i < 4; i++) {
    int dev = (motion[i] >> 12);
    int a = motion[i] & 0xfff;
    if(dev) {
      dev--;

      if((dev == which) && (a < 32) && ((a>>1) == axis)) {
        sdlMotionButtons[i] = (a & 1) ? (value > 16384) : (value < -16384);
      }
    }
  }  
}

bool sdlCheckJoyKey(int key)
{
  int dev = (key >> 12) - 1;
  int what = key & 0xfff;

  if(what >= 128) {
    // joystick button
    int button = what - 128;

    if(button >= SDL_JoystickNumButtons(sdlDevices[dev]))
      return false;
  } else if (what < 0x20) {
    // joystick axis    
    what >>= 1;
    if(what >= SDL_JoystickNumAxes(sdlDevices[dev]))
      return false;
  } else if (what < 0x30) {
    // joystick hat
    what = (what & 15);
    what >>= 2;
    if(what >= SDL_JoystickNumHats(sdlDevices[dev]))
      return false;
  }

  // no problem found
  return true;
}

void sdlCheckKeys()
{
  sdlNumDevices = SDL_NumJoysticks();

  if(sdlNumDevices)
    sdlDevices = (SDL_Joystick **)calloc(1,sdlNumDevices *
                                         sizeof(SDL_Joystick **));
  int i;

  bool usesJoy = false;

  for(int j = 0; j < 4; j++) {
    for(i = 0; i < 12; i++) {
      int dev = joypad[j][i] >> 12;
      if(dev) {
        dev--;
        bool ok = false;
        
        if(sdlDevices) {
          if(dev < sdlNumDevices) {
            if(sdlDevices[dev] == NULL) {
              sdlDevices[dev] = SDL_JoystickOpen(dev);
            }
            
            ok = sdlCheckJoyKey(joypad[j][i]);
          } else
            ok = false;
        }
        
        if(!ok)
          joypad[j][i] = defaultJoypad[i];
        else
          usesJoy = true;
      }
    }
  }

  for(i = 0; i < 4; i++) {
    int dev = motion[i] >> 12;
    if(dev) {
      dev--;
      bool ok = false;
      
      if(sdlDevices) {
        if(dev < sdlNumDevices) {
          if(sdlDevices[dev] == NULL) {
            sdlDevices[dev] = SDL_JoystickOpen(dev);
          }
          
          ok = sdlCheckJoyKey(motion[i]);
        } else
          ok = false;
      }
      
      if(!ok)
        motion[i] = defaultMotion[i];
      else
        usesJoy = true;
    }
  }

  if(usesJoy)
    SDL_JoystickEventState(SDL_ENABLE);
}

void sdlPollEvents()
{
#ifdef AOS_SDL2
  SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
#endif

  SDL_Event event;
  while(SDL_PollEvent(&event)) {
    switch(event.type) {
    case SDL_QUIT:
      emulating = 0;
      break;

#ifndef AOS_SDL2
    case SDL_VIDEORESIZE:
    // markus sdl resize
        if (openGL)
         {
            SDL_SetVideoMode(event.resize.w, event.resize.h, 16,
                       SDL_OPENGL | SDL_RESIZABLE |
                       (fullscreen ? SDL_FULLSCREEN : 0));
            sdlOpenGLInit(event.resize.w, event.resize.h);
        }
     break;
    case SDL_ACTIVEEVENT:
      if(pauseWhenInactive && (event.active.state & SDL_APPINPUTFOCUS)) {
        active = event.active.gain;
        if(active) {
          if(!paused) {
            if(emulating)
              soundResume();
          }
        } else {
          wasPaused = true;
          if(pauseWhenInactive) {
            if(emulating)
              soundPause();
          }

          memset(delta,255,sizeof(delta));
        }
      }
      break;
#else
//markus resieze opengl sdl2
    case SDL_WINDOWEVENT:
      switch (event.window.event) {

        case SDL_WINDOWEVENT_RESIZED:
          if (openGL)
            sdlOpenGLScaleWithAspect(event.window.data1, event.window.data2);
          break;
     }
#endif
    case SDL_MOUSEMOTION:
    case SDL_MOUSEBUTTONUP:
      if(fullscreen) {
        SDL_ShowCursor(SDL_ENABLE);
        mouseCounter = 120;
      }
      break;
    case SDL_MOUSEBUTTONDOWN:
#ifdef AOS_SDL2
      if (event.button.clicks > 1) {
        fullscreen = !fullscreen;

        if (SDL_FULL) SDL_SetWindowFullscreen(window, (flags|(fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0)));
           else SDL_SetWindowFullscreen(window, (flags|(fullscreen ? SDL_WINDOW_FULLSCREEN : 0)));
        //opengl
        if (openGL)
          {
            if (fullscreen)
              sdlOpenGLScaleWithAspect(desktopWidth, desktopHeight);
            else
              sdlOpenGLScaleWithAspect(destWidth, destHeight);
        }
#ifndef __AMIGAOS4__
         SDL_RenderClear(renderer);
#else
         if (!fullscreen) AmigaOS_ScreenTitle(window,filename); //overwriting the WB string generated by SDL
#endif //AOS4
      }
#endif
      if(fullscreen) {
        SDL_ShowCursor(SDL_ENABLE);
        mouseCounter = 120;
      }
      break;
    case SDL_JOYHATMOTION:
      sdlUpdateJoyHat(event.jhat.which,
                      event.jhat.hat,
                      event.jhat.value);
      break;
    case SDL_JOYBUTTONDOWN:
    case SDL_JOYBUTTONUP:
      sdlUpdateJoyButton(event.jbutton.which,
                         event.jbutton.button,
                         event.jbutton.state == SDL_PRESSED);
      break;
    case SDL_JOYAXISMOTION:
      sdlUpdateJoyAxis(event.jaxis.which,
                       event.jaxis.axis,
                       event.jaxis.value);
      break;
#ifdef AOS_SDL2
      //SDL_DROP todo!!!
     case SDL_DROPFILE: 
       {
          const char    *extensions[] =
            { ".gba",".gb",".gbc",".zip",
              ".GBA",".GB",".GBC",".ZIP",
              NULL
            };

             char *ext;
             SDL_bool bingo=SDL_FALSE;
            //check extension
             ext = strrchr(event.drop.file, '.');
            if (ext) {
                for (int i = 0; extensions[i]; i++)
                 {
                    if (strcmp(extensions[i],ext) == 0 )
                      bingo = SDL_TRUE;
                 }
               ext=NULL;
            }
            if (bingo) {
                dropped_file = event.drop.file;
                if (debug_fprintf) fprintf (stderr,"SDL drop file %s go go go .. \n",dropped_file);
                emulating = 0;
             }
       }
       break;
#endif
    case SDL_KEYDOWN:
#ifndef AOS_SDL2 
      sdlUpdateKey(event.key.keysym.sym, true);
#else
      if (!event.key.keysym.mod) sdlUpdateKey(event.key.keysym.sym, true);
#endif
      break;
      case SDL_KEYUP:
        switch(event.key.keysym.sym) {
         case SDLK_r:
         if(!(event.key.keysym.mod & MOD_NOCTRL) &&
           (event.key.keysym.mod & KMOD_CTRL)) {
          if(emulating) {
            emulator.emuReset();

            systemScreenMessage("Reset");
          }
        }
        break;

      case SDLK_b:
        if(!(event.key.keysym.mod & MOD_NOCTRL) &&
           (event.key.keysym.mod & KMOD_CTRL)) {
          if(emulating && emulator.emuReadMemState && rewindMemory 
             && rewindCount) {
            rewindPos = (rewindPos - 1) & 7;
            emulator.emuReadMemState(&rewindMemory[REWIND_SIZE*rewindPos], 
                                     REWIND_SIZE);
            rewindCount--;
            rewindCounter = 0;
            systemScreenMessage("Rewind");
          }
        }
        break;
      case SDLK_p:
        if(!(event.key.keysym.mod & MOD_NOCTRL) &&
           (event.key.keysym.mod & KMOD_CTRL)) {
          paused = !paused;
          SDL_PauseAudio(paused);
          if(paused)
            wasPaused = true;
        }
        break;
      case SDLK_ESCAPE:
        emulating = 0;
        break;
      case SDLK_f:
        if(!(event.key.keysym.mod & MOD_NOCTRL) &&
           (event.key.keysym.mod & KMOD_CTRL)) {
          fullscreen = !fullscreen;
#ifdef AOS_SDL2 

         if (SDL_FULL) SDL_SetWindowFullscreen(window, (flags|(fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0)));
             else SDL_SetWindowFullscreen(window, (flags|(fullscreen ? SDL_WINDOW_FULLSCREEN : 0)));

         if (openGL)
           {
            if (fullscreen)
              sdlOpenGLScaleWithAspect(desktopWidth, desktopHeight);
            else
              sdlOpenGLScaleWithAspect(destWidth, destHeight);
          }

#ifndef __AMIGAOS4__
          SDL_RenderClear(renderer);
#else
          if (!fullscreen) AmigaOS_ScreenTitle(window,filename); //overwriting the WB string generated by SDL
#endif //AOS4

#else
          if(fullscreen)
            flags |= SDL_FULLSCREEN;
           if(openGL)
            flags |= SDL_OPENGL | SDL_RESIZABLE;
          SDL_SetVideoMode(destWidth, destHeight, systemColorDepth, flags);

         if(openGL)
         sdlOpenGLInit(destWidth, destHeight);


#endif //AOS_SDL2
        }
        break;
#ifndef __AMIGAOS4__
      case SDLK_F11:
        if(dbgMain != debuggerMain) {
          if(armState) {
            armNextPC -= 4;
            reg[15].I -= 4;
          } else {
            armNextPC -= 2;
            reg[15].I -= 2;
          }
        }
        debugger = true;
        break;
#endif
      case SDLK_F1:
      case SDLK_F2:
      case SDLK_F3:
      case SDLK_F4:
      case SDLK_F5:
      case SDLK_F6:
      case SDLK_F7:
      case SDLK_F8:
      case SDLK_F9:
      case SDLK_F10:
        if(!(event.key.keysym.mod & MOD_NOSHIFT) &&
           (event.key.keysym.mod & KMOD_SHIFT)) {
          sdlWriteState(event.key.keysym.sym-SDLK_F1);
        } else if(!(event.key.keysym.mod & MOD_KEYS)) {
          sdlReadState(event.key.keysym.sym-SDLK_F1);
        }
        break;
      case SDLK_1:
      case SDLK_2:
      case SDLK_3:
      case SDLK_4:
        if(!(event.key.keysym.mod & MOD_NOALT) &&
           (event.key.keysym.mod & KMOD_ALT)) {
          char *disableMessages[4] = 
            { "autofire A disabled",
              "autofire B disabled",
              "autofire R disabled",
              "autofire L disabled"};
          char *enableMessages[4] = 
            { "autofire A",
              "autofire B",
              "autofire R",
              "autofire L"};
          int mask = 1 << (event.key.keysym.sym - SDLK_1);
    if(event.key.keysym.sym > SDLK_2)
      mask <<= 6;
          if(autoFire & mask) {
            autoFire &= ~mask;
            systemScreenMessage(disableMessages[event.key.keysym.sym - SDLK_1]);
          } else {
            autoFire |= mask;
            systemScreenMessage(enableMessages[event.key.keysym.sym - SDLK_1]);
          }
        } if(!(event.key.keysym.mod & MOD_NOCTRL) &&
             (event.key.keysym.mod & KMOD_CTRL)) {
          int mask = 0x0100 << (event.key.keysym.sym - SDLK_1);
          layerSettings ^= mask;
          layerEnable = DISPCNT & layerSettings;
          CPUUpdateRenderBuffers(false);
        }
        break;
      case SDLK_5:
      case SDLK_6:
      case SDLK_7:
      case SDLK_8:
        if(!(event.key.keysym.mod & MOD_NOCTRL) &&
           (event.key.keysym.mod & KMOD_CTRL)) {
          int mask = 0x0100 << (event.key.keysym.sym - SDLK_1);
          layerSettings ^= mask;
          layerEnable = DISPCNT & layerSettings;
        }
        break;
      case SDLK_n:
        if(!(event.key.keysym.mod & MOD_NOCTRL) &&
           (event.key.keysym.mod & KMOD_CTRL)) {
          if(paused)
            paused = false;
          pauseNextFrame = true;
        }
        break;
      default:
        break;
      }
#ifndef AOS_SDL2 
      sdlUpdateKey(event.key.keysym.sym, false);
#else
      if (!event.key.keysym.mod) sdlUpdateKey(event.key.keysym.sym, false);
#endif
      break;
    }
  }
}

void usage(char *cmd)
{
  printf("%s [option ...] file\n", cmd);
  printf("\
\n\
Options:\n\
  -1, --video-1x               1x\n\
  -2, --video-2x               2x\n\
  -3, --video-3x               3x\n\
  -4, --video-4x               4x\n\
");
#ifndef __AMIGAOS4__
  printf("\
  -O, --opengl=MODE            Set OpenGL texture filter\n\
      --no-opengl               0 - Disable OpenGL\n\
      --opengl-nearest          1 - No filtering\n\
      --opengl-bilinear         2 - Bilinear filtering\n\

");
#endif
  printf("\
  -F, --fullscreen             Full screen\n\
  -G, --gdb=PROTOCOL           GNU Remote Stub mode:\n\
                                tcp      - use TCP at port 55555\n\
                                tcp:PORT - use TCP at port PORT\n\
                                pipe     - use pipe transport\n\
  -N, --no-debug               Don't parse debug information\n\
  -S, --flash-size=SIZE        Set the Flash size\n\
      --flash-64k               0 -  64K Flash\n\
      --flash-128k              1 - 128K Flash\n\
  -T, --throttle=THROTTLE      Set the desired throttle (5...1000)\n\
  -b, --bios=BIOS              Use given bios file\n\
  -c, --config=FILE            Read the given configuration file\n\
  -d, --debug                  Enter debugger\n\
  -f, --filter=FILTER          Select filter:\n\
      --filter-normal            0 - normal mode\n\
      --filter-tv-mode           1 - TV Mode\n\
      --filter-2xsai             2 - 2xSaI\n\
      --filter-super-2xsai       3 - Super 2xSaI\n\
      --filter-super-eagle       4 - Super Eagle\n\
      --filter-pixelate          5 - Pixelate\n\
      --filter-motion-blur       6 - Motion Blur\n\
      --filter-advmame           7 - AdvanceMAME Scale2x\n\
      --filter-simple2x          8 - Simple2x\n\
      --filter-bilinear          9 - Bilinear\n\
      --filter-bilinear+        10 - Bilinear Plus\n\
      --filter-scanlines        11 - Scanlines\n\
      --filter-hq2x             12 - hq2x\n\
      --filter-lq2x             13 - lq2x\n\
  -h, --help                   Print this help\n\
  -i, --ips=PATCH              Apply given IPS patch\n\
  -p, --profile=[HERTZ]        Enable profiling\n\
  -s, --frameskip=FRAMESKIP    Set frame skip (0...9)\n\
");
  printf("\
  -t, --save-type=TYPE         Set the available save type\n\
      --save-auto               0 - Automatic (EEPROM, SRAM, FLASH)\n\
      --save-eeprom             1 - EEPROM\n\
      --save-sram               2 - SRAM\n\
      --save-flash              3 - FLASH\n\
      --save-sensor             4 - EEPROM+Sensor\n\
      --save-none               5 - NONE\n\
  -v, --verbose=VERBOSE        Set verbose logging (trace.log)\n\
                                  1 - SWI\n\
                                  2 - Unaligned memory access\n\
                                  4 - Illegal memory write\n\
                                  8 - Illegal memory read\n\
                                 16 - DMA 0\n\
                                 32 - DMA 1\n\
                                 64 - DMA 2\n\
                                128 - DMA 3\n\
                                256 - Undefined instruction\n\
                                512 - AGBPrint messages\n\
\n\
Long options only:\n\
      --agb-print              Enable AGBPrint support\n\
      --auto-frameskip         Enable auto frameskipping\n\
      --ifb-none               No interframe blending\n\
      --ifb-motion-blur        Interframe motion blur\n\
      --ifb-smart              Smart interframe blending\n\
      --no-agb-print           Disable AGBPrint support\n\
      --no-auto-frameskip      Disable auto frameskipping\n\
      --no-ips                 Do not apply IPS patch\n\
      --no-pause-when-inactive Don't pause when inactive\n\
      --no-rtc                 Disable RTC support\n\
      --no-show-speed          Don't show emulation speed\n\
      --no-throttle            Disable thrrotle\n\
      --mute                   Disable sound\n\
");
#ifdef AOS_SDL2
  printf("\
      --sdl2wfd                Enable SDL2/SDL_WINDOW_FULLSCREEN_DESKTOP\n\
");
#endif
  printf("\
      --pause-when-inactive    Pause when inactive\n\
      --rtc                    Enable RTC support\n\
      --show-speed-normal      Show emulation speed\n\
      --show-speed-detailed    Show detailed speed data\n\
");

}
//markus SDLROP Window
#ifdef AOS_SDL2
void VBA_DropWindow (void)
{
    //extension
    const char    *extensions[] =
    { ".gba",".gb",".gbc",".zip",
      ".GBA",".GB",".GBC",".ZIP",
      NULL
    };

    char *ext;
    SDL_bool done,bingo;
    SDL_Window *window;
    SDL_Event event;                        // Declare event handle

    SDL_Init(SDL_INIT_VIDEO);               // SDL2 initialization

    window = SDL_CreateWindow(  // Create a window
        "Please drop the rom file on window",
	SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 400, 270, 0);

    SDL_RWops *pixelsWop = SDL_RWFromConstMem((const unsigned char *)logovba, sizeof(logovba));
    SDL_Surface *image = SDL_LoadBMP_RW(pixelsWop, 1);

    SDL_Renderer * renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Texture * texture = SDL_CreateTextureFromSurface(renderer, image);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    // Check that the window was successfully made
    if (window == NULL) {
        // In the event that the window could not be made...
        SDL_Log("Could not create window: %s", SDL_GetError());
        SDL_Quit();
    }

    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

    done = SDL_FALSE;
    bingo = SDL_FALSE;
    while (!done) {                         // Program loop
        while (!done && SDL_PollEvent(&event)) {
            switch (event.type) {
                case (SDL_QUIT): {          // In case of exit
                    done = SDL_TRUE;
                    break;
                }
		case (SDL_KEYDOWN):
		 if(event.key.keysym.sym == SDLK_q) {
		 done = SDL_TRUE;
		}
		if(event.key.keysym.sym == SDLK_ESCAPE) {
		done = SDL_TRUE;
		}	
 		    break;
                //}
               case (SDL_DROPFILE): {      // In case if dropped file
		    //check extension
		    ext = strrchr(event.drop.file, '.');
		    if (ext) 
		    {
		      for (int i = 0; extensions[i]; i++)
			{
				if (strcmp(extensions[i],ext) == 0 )
				    bingo = SDL_TRUE;
			}
		    }
		    if (!bingo)
		    {
#ifdef __AMIGAOS4__
	                 SDL_ShowSimpleMessageBox(
                         SDL_MESSAGEBOX_ERROR,
                         AMIGA_VERSION_SIGN,
                         "!!!  FILE IS  not ROM !!!",
                         window);
#else
	                 SDL_ShowSimpleMessageBox(
                         SDL_MESSAGEBOX_ERROR,
                         "File dropped on window",
                         "!!!  FILE IS  not ROM !!!",
                         window);
#endif
		    }
		    else 
		    {
                     dropped_file = event.drop.file;
		     done = SDL_TRUE;
		    }

                 }
                    break;
            }
        }
        SDL_Delay(50);
    }

    // Close and destroy the window
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(image);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);        

// Clean up fixme!!!!
    SDL_Quit();                       
    if (!dropped_file) 
       exit(0);

}
#endif
//

int main(int argc, char **argv)
{
#ifndef __AMIGAOS4__
  debug_fprintf = 1; //simple stupid but it works
#endif

  char buf[1024];
  if (debug_fprintf) fprintf(stderr, "VisualBoyAdvance version %s [SDL2]\n", VERSION);

  arg0 = argv[0];
  
  captureDir[0] = 0;
  saveDir[0] = 0;
  batteryDir[0] = 0;
  ipsname[0] = 0;
  
  int op = -1;
  struct stat s;

  frameSkip = 2;
  gbBorderOn = 0;

  parseDebug = true;





#ifndef __AMIGAOS4__
  homeDir = getenv("HOME");
#endif
  snprintf(buf, 1024, "%s/%s", homeDir, DOT_DIR);
  /* Make dot dir if not existent */
  if (stat(buf, &s) == -1 || !S_ISDIR(s.st_mode))
    mkdir(buf, 0755);
#ifdef __AMIGAOS4__
//saves
  snprintf(buf, 1024, "%s/%s/%s", homeDir, DOT_DIR, DOT_SAV);
  /* Make dot dir if not existent */
  if (stat(buf, &s) == -1 || !S_ISDIR(s.st_mode))
    mkdir(buf, 0755);
//ram
  snprintf(buf, 1024, "%s/%s/%s", homeDir, DOT_DIR, DOT_RAM);
  /* Make dot dir if not existent */
  if (stat(buf, &s) == -1 || !S_ISDIR(s.st_mode))
    mkdir(buf, 0755);
//screenshots
  snprintf(buf, 1024, "%s/%s/%s", homeDir, DOT_DIR, DOT_PIC);
  /* Make dot dir if not existent */
  if (stat(buf, &s) == -1 || !S_ISDIR(s.st_mode))
    mkdir(buf, 0755);
//bios
  snprintf(buf, 1024, "%s/%s/%s", homeDir, DOT_DIR, DOT_BIOS);
  /* Make dot dir if not existent */
  if (stat(buf, &s) == -1 || !S_ISDIR(s.st_mode))
    mkdir(buf, 0755);
#endif

  sdlReadPreferences();

  sdlPrintUsage = 0;


#ifdef AOS_SDL2
//DROP 
   if ((argc < 2 ) && (!dropped_file) && droppwindow)
        VBA_DropWindow();

   if (dropped_file) {
       argc=2;
       argv[1]=(char*)dropped_file;
       dropped_file = NULL;
   }
#endif
//AOS4
#ifdef __AMIGAOS4__
    atexit(AmigaOS_Close); // avoid unfreeded resources
    if (AmigaOS_Open(argc, argv) == -1)  return 1;
  {

    int new_argc;
    char **new_argv;

    if (argc<=1) {
     AmigaOS_ParseArg(argc, argv, &new_argc, &new_argv);
        /* nie wybrano nic z wyboru pliku */
        if (new_argc==0 && new_argv==NULL)
          return 0;

    argc=new_argc;
    argv=new_argv;
    }

  }
#endif

  while((op = getopt_long(argc,
                          argv,
#ifndef __AMIGAOS4__
                           "FNO:T:Y:G:D:b:c:df:hi:p::s:t:v:1234",
#else
                           "FNT:Y:G:D:b:c:df:hi:p::s:t:v:1234",
#endif
                          sdlOptions,
                          NULL)) != -1) {
    switch(op) {
    case 0:
      // long option already processed by getopt_long
      break;
    case 'b':
      useBios = true;
      if(optarg == NULL) {
        if (debug_fprintf) fprintf(stderr, "Missing BIOS file name\n");
        exit(-1);
      }
      strcpy(biosFileName, optarg);
      break;
    case 'c':
      {
        if(optarg == NULL) {
          if (debug_fprintf) fprintf(stderr, "Missing config file name\n");
          exit(-1);

}
        FILE *f = fopen(optarg, "r");
        if(f == NULL) {
          if (debug_fprintf) fprintf(stderr, "File not found %s\n", optarg);
          exit(-1);
        }
        sdlReadPreferences(f);
        fclose(f);
      }
      break;
    case 'd':
      debugger = true;
      break;
    case 'h':
      sdlPrintUsage = 1;
      break;
    case 'i':
      if(optarg == NULL) {
        if (debug_fprintf) fprintf(stderr, "Missing IPS name\n");
        exit(-1);
        strcpy(ipsname, optarg);
      }
      break;
    case 'G':
      dbgMain = remoteStubMain;
      dbgSignal = remoteStubSignal;
      dbgOutput = remoteOutput;
      debugger = true;
      debuggerStub = true;
      if(optarg) {
        char *s = optarg;
        if(strncmp(s,"tcp:", 4) == 0) {
          s+=4;
          int port = atoi(s);
          remoteSetProtocol(0);
          remoteSetPort(port);
        } else if(strcmp(s,"tcp") == 0) {
          remoteSetProtocol(0);
        } else if(strcmp(s, "pipe") == 0) {
          remoteSetProtocol(1);
        } else {
          if (debug_fprintf) fprintf(stderr, "Unknown protocol %s\n", s);
          exit(-1);
        }
      } else {
        remoteSetProtocol(0);
      }
      break;
    case 'N':
      parseDebug = false;
      break;
    case 'D':
      if(optarg) {
        systemDebug = atoi(optarg);
      } else {
        systemDebug = 1;
      }
      break;
    case 'F':
      fullscreen = 1;
      mouseCounter = 120;
      break;
    case 'f':
      if(optarg) {
        filter = atoi(optarg);
      } else {
        filter = 0;
      }
      break;
    case 'p':
#ifdef PROFILING
      if(optarg) {
        cpuEnableProfiling(atoi(optarg));
      } else
        cpuEnableProfiling(100);
#endif
      break;
    case 'S':
      sdlFlashSize = atoi(optarg);
      if(sdlFlashSize < 0 || sdlFlashSize > 1)
        sdlFlashSize = 0;
      break;
    case 's':
      if(optarg) {
        int a = atoi(optarg);
        if(a >= 0 && a <= 9) {
          gbFrameSkip = a;
          frameSkip = a;
        }
      } else {
        frameSkip = 2;
        gbFrameSkip = 0;
      }
      break;
    case 't':
      if(optarg) {
        int a = atoi(optarg);
        if(a < 0 || a > 5)
          a = 0;
        cpuSaveType = a;
      }
      break;
    case 'T':
      if(optarg) {
        int t = atoi(optarg);
        if(t < 5 || t > 1000)
          t = 0;
        throttle = t;
      }
      break;
    case 'v':
      if(optarg) {
        systemVerbose = atoi(optarg);
      } else
        systemVerbose = 0;
      break;
    case '1':
      sizeOption = 0;
      break;
    case '2':
      sizeOption = 1;
      break;
    case '3':
      sizeOption = 2;
      break;
    case '4':
      sizeOption = 3;
      break;
#ifndef __AMIGAOS4__
    case 'O':
      if(optarg) {
       openGL = atoi(optarg);
        if (openGL < 0 || openGL > 2)
         openGL = 1;
     } else
        openGL = 0;
    break;
#endif
    case '?':
      sdlPrintUsage = 1;
      break;
    }
  }

  if(sdlPrintUsage) {
    usage(argv[0]);
    exit(-1);
  }

#ifdef MMX
  if(disableMMX)
    cpu_mmx = 0;
#endif

  if(rewindTimer)
    rewindMemory = (char *)malloc(8*REWIND_SIZE);

  if(sdlFlashSize == 0)
    flashSetSize(0x10000);
  else
    flashSetSize(0x20000);

//markus
#ifdef AOS_SDL2 
    sdlReadDesktopVideoMode();
#endif

  if(fullscreen_window)
     SDL_FULL = (char *)"TRUE";
//end markus

  rtcEnable(sdlRtcEnable ? true : false);
  agbPrintEnable(sdlAgbPrint ? true : false);

  if(!debuggerStub) {
    if(optind >= argc) {
      systemMessage(0,"Missing image name");
      usage(argv[0]);
      exit(-1);
    }
  }

  if(filter) {
    if(!openGL)
     sizeOption = 1;
  }

  for(int i = 0; i < 24;) {
    systemGbPalette[i++] = (0x1f) | (0x1f << 5) | (0x1f << 10);
    systemGbPalette[i++] = (0x15) | (0x15 << 5) | (0x15 << 10);
    systemGbPalette[i++] = (0x0c) | (0x0c << 5) | (0x0c << 10);
    systemGbPalette[i++] = 0;
  }

  systemSaveUpdateCounter = SYSTEM_SAVE_NOT_UPDATED;

  if(optind < argc) {
    char *szFile = argv[optind];
    u32 len = strlen(szFile);
    if (len > SYSMSG_BUFFER_SIZE) 
    {
      if (debug_fprintf) fprintf(stderr,"%s :%s: File name too long\n",argv[0],szFile);
      exit(-1);
    }

// markus file drop - starts here
#ifdef AOS_SDL2
run_again_dropfile:

       if (dropped_file) {
        strcpy(szFile, dropped_file);
        dropped_file = NULL;
       }
#endif
//
    utilGetBaseName(szFile, filename);
    char *p = strrchr(filename, '.');

    if(p)
      *p = 0;

    if(ipsname[0] == 0)
      sprintf(ipsname, "%s.ips", filename);
    
    bool failed = false;

    IMAGE_TYPE type = utilFindType(szFile);

    if(type == IMAGE_UNKNOWN) {
      systemMessage(0, "Unknown file type %s", szFile);
      exit(-1);
    }
    cartridgeType = (int)type;
    
    if(type == IMAGE_GB) {
      failed = !gbLoadRom(szFile);
      if(!failed) {
        gbGetHardwareType();
        
        // used for the handling of the gb Boot Rom
        if (gbHardware & 5)
        {
			    char tempName[0x800];
			    strcpy(tempName, arg0);
			    char *p = strrchr(tempName, '\\');
			    if(p) { *p = 0x00; }
			    strcat(tempName, "\\DMG_ROM.bin");
          if (debug_fprintf) fprintf(stderr, "%s\n", tempName);
			    gbCPUInit(tempName, useBios);
        }
        else useBios = false;
        
        gbReset();
        cartridgeType = IMAGE_GB;
        emulator = GBSystem;
        if(sdlAutoIPS) {
          int size = gbRomSize;
          utilApplyIPS(ipsname, &gbRom, &size);
          if(size != gbRomSize) {
            extern bool gbUpdateSizes();
            gbUpdateSizes();
            gbReset();
          }
        }
      }
    } else if(type == IMAGE_GBA) {
      int size = CPULoadRom(szFile);
      failed = (size == 0);
      if(!failed) {
        sdlApplyPerImagePreferences();

        doMirroring(mirroringEnable);
        
        cartridgeType = 0;
        emulator = GBASystem;

        /* disabled due to problems
        if(removeIntros && rom != NULL) {
          WRITE32LE(&rom[0], 0xea00002e);
        }
        */
        
        CPUInit(biosFileName, useBios);
        CPUReset();
        if(sdlAutoIPS) {
          int size = 0x2000000;
          utilApplyIPS(ipsname, &rom, &size);
          if(size != 0x2000000) {
            CPUReset();
          }
        }
      }
    }
    
    if(failed) {
      systemMessage(0, "Failed to load file %s", szFile);
      exit(-1);
    }
  } else {
    cartridgeType = 0;
    strcpy(filename, "gnu_stub");
    rom = (u8 *)malloc(0x2000000);
    workRAM = (u8 *)calloc(1, 0x40000);
    bios = (u8 *)calloc(1,0x4000);
    internalRAM = (u8 *)calloc(1,0x8000);
    paletteRAM = (u8 *)calloc(1,0x400);
    vram = (u8 *)calloc(1, 0x20000);
    oam = (u8 *)calloc(1, 0x400);
    pix = (u8 *)calloc(1, 4 * 241 * 162);
    ioMem = (u8 *)calloc(1, 0x400);

    emulator = GBASystem;
    
    CPUInit(biosFileName, useBios);
    CPUReset();    
  }
  
  sdlReadBattery();
  
  if(debuggerStub) 
    remoteInit();
  
    flags = SDL_INIT_VIDEO|SDL_INIT_AUDIO|
    SDL_INIT_TIMER|SDL_INIT_NOPARACHUTE;
  // markus mute - loses synchronization with SDL, opengl works fine
  if(mute) soundOffFlag = true;

  if(soundOffFlag)
    flags ^= SDL_INIT_AUDIO;
  
  if(SDL_Init(flags)) {
    systemMessage(0, "Failed to init SDL: %s", SDL_GetError());
    exit(-1);
  }

#ifdef AOS_SDL2
 if(SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0) {
#else
  if(SDL_InitSubSystem(SDL_INIT_JOYSTICK)) {
#endif
    systemMessage(0, "Failed to init joystick support: %s", SDL_GetError());
  }
  
  sdlCheckKeys();
  
  if(cartridgeType == 0) {
    srcWidth = 240;
    srcHeight = 160;
    systemFrameSkip = frameSkip;
  } else if (cartridgeType == 1) {
    if(gbBorderOn) {
      srcWidth = 256;
      srcHeight = 224;
      gbBorderLineSkip = 256;
      gbBorderColumnSkip = 48;
      gbBorderRowSkip = 40;
    } else {      
      srcWidth = 160;
      srcHeight = 144;
      gbBorderLineSkip = 160;
      gbBorderColumnSkip = 0;
      gbBorderRowSkip = 0;
    }
    systemFrameSkip = gbFrameSkip;
  } else {
    srcWidth = 320;
    srcHeight = 240;
  }
  
  destWidth = (sizeOption+1)*srcWidth;
  destHeight = (sizeOption+1)*srcHeight;

#ifdef AOS_SDL2
  flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_WINDOW_RESIZABLE ;
  if(openGL) {
     flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;

     if (SDL_FULL) window = SDL_CreateWindow("VBA", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, destWidth, destHeight, (flags|(fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0)));
       else window = SDL_CreateWindow("VBA", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, destWidth, destHeight, (flags|(fullscreen ? SDL_WINDOW_FULLSCREEN : 0)));

     context = SDL_GL_CreateContext(window);
     SDL_GL_SetSwapInterval(1);
     SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
     SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
     SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
     SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    } else {

     if (SDL_FULL) SDL_CreateWindowAndRenderer(destWidth, destHeight, (flags|(fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0)), &window, &renderer);
       else SDL_CreateWindowAndRenderer(destWidth, destHeight, (flags|(fullscreen ? SDL_WINDOW_FULLSCREEN : 0)), &window, &renderer);
   }


  SDL_SetWindowMinimumSize(window, destWidth, destHeight);
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

  surface = SDL_CreateRGBSurface(0, destWidth, destHeight, 32,
                                   0x00FF0000, 0x0000FF00,
                                   0x000000FF, 0xFF000000);
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                destWidth, destHeight);
  SDL_RenderClear(renderer); //markus renderer dont need for opengl ? fixme !!!

  if(window == NULL) {
#else
  flags = SDL_ANYFORMAT | (fullscreen ? SDL_FULLSCREEN : 0);
   if(openGL) {
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
     flags |= SDL_OPENGL | SDL_RESIZABLE;
   } else
      flags |= SDL_HWSURFACE | SDL_DOUBLEBUF;

  surface = SDL_SetVideoMode(destWidth, destHeight, 16, flags);

  if(surface == NULL) {
#endif
    systemMessage(0, "Failed to set window");
    SDL_Quit();
    exit(-1);
  }


  systemRedShift = sdlCalculateShift(surface->format->Rmask);
  systemGreenShift = sdlCalculateShift(surface->format->Gmask);
  systemBlueShift = sdlCalculateShift(surface->format->Bmask);
  
  systemColorDepth = surface->format->BitsPerPixel;

  if(systemColorDepth == 15)
    systemColorDepth = 16;

  if(systemColorDepth != 16 && systemColorDepth != 24 &&
     systemColorDepth != 32) {
    if (debug_fprintf) fprintf(stderr,"Unsupported color depth '%d'.\nOnly 16, 24 and 32 bit color depths are supported\n", systemColorDepth);
    exit(-1);
  }

#ifndef C_CORE
  sdlMakeStretcher(srcWidth);
#else
  switch(systemColorDepth) {
  case 16:
    sdlStretcher = sdlStretcher16[sizeOption];
    break;
  case 24:
    sdlStretcher = sdlStretcher24[sizeOption];
    break;
  case 32:
    sdlStretcher = sdlStretcher32[sizeOption];
    break;
  default:
    if (debug_fprintf) fprintf(stderr, "Unsupported resolution: %d\n", systemColorDepth);
    exit(-1);
  }
#endif

  if (debug_fprintf) fprintf(stderr,"Color depth: %d\n", systemColorDepth);
//markus
  if (openGL)
     if (debug_fprintf) fprintf(stderr,"OpenGL initialized\n");

  if(systemColorDepth == 16) {
    if(sdlCalculateMaskWidth(surface->format->Gmask) == 6) {
      Init_2xSaI(565);
    } else {
      Init_2xSaI(555);
    }
    srcPitch = srcWidth * 2+4;
  } else {
    if(systemColorDepth != 32)
      filterFunction = NULL;
    Init_2xSaI(32);
    if(systemColorDepth == 32)
      srcPitch = srcWidth*4 + 4;
    else
      srcPitch = srcWidth*3;
  }
  utilUpdateSystemColorMaps();

  if(systemColorDepth != 32) {
    switch(filter) {
    case 0:
      filterFunction = NULL;
      break;
    case 1:
      filterFunction = ScanlinesTV;
      break;
    case 2:
      filterFunction = _2xSaI;
      break;
    case 3:
      filterFunction = Super2xSaI;
      break;
    case 4:
      filterFunction = SuperEagle;
      break;
    case 5:
      filterFunction = Pixelate;
      break;
    case 6:
      filterFunction = MotionBlur;
      break;
    case 7:
      filterFunction = AdMame2x;
      break;
    case 8:
      filterFunction = Simple2x;
      break;
    case 9:
      filterFunction = Bilinear;
      break;
    case 10:
      filterFunction = BilinearPlus;
      break;
    case 11:
      filterFunction = Scanlines;
      break;
    case 12:
      filterFunction = hq2x;
      break;
    case 13:
      filterFunction = lq2x;
      break;
    default:
      filterFunction = NULL;
      break;
    }
  } else {
    switch(filter) {
    case 0:
      filterFunction = NULL;
      break;
    case 1:
      filterFunction = ScanlinesTV32;
      break;
    case 2:
      filterFunction = _2xSaI32;
      break;
    case 3:
      filterFunction = Super2xSaI32;
      break;
    case 4:
      filterFunction = SuperEagle32;
      break;
    case 5:
      filterFunction = Pixelate32;
      break;
    case 6:
      filterFunction = MotionBlur32;
      break;
    case 7:
      filterFunction = AdMame2x32;
      break;
    case 8:
      filterFunction = Simple2x32;
      break;
    case 9:
      filterFunction = Bilinear32;
      break;
    case 10:
      filterFunction = BilinearPlus32;
      break;
    case 11:
      filterFunction = Scanlines32;
      break;
    case 12:
      filterFunction = hq2x32;
      break;
    case 13:
      filterFunction = lq2x32;
      break;
    default:
      filterFunction = NULL;
      break;
    }
  }

  if(systemColorDepth == 16) {
    switch(ifbType) {
    case 0:
    default:
      ifbFunction = NULL;
      break;
    case 1:
      ifbFunction = MotionBlurIB;
      break;
    case 2:
      ifbFunction = SmartIB;
      break;
    }
  } else if(systemColorDepth == 32) {
    switch(ifbType) {
    case 0:
    default:
      ifbFunction = NULL;
      break;
    case 1:
      ifbFunction = MotionBlurIB32;
      break;
    case 2:
      ifbFunction = SmartIB32;
      break;
    }
  } else
    ifbFunction = NULL;

  if(delta == NULL) {
    delta = (u8*)malloc(322*242*4);
    memset(delta, 255, 322*242*4);
  }

 if(openGL) {
    filterPix = (u8 *)calloc(1, 4 * 4 * 256 * 240);
    sdlOpenGLInit(destWidth, destHeight);
  }

  emulating = 1;
  renderedFrames = 0;

  if(!soundOffFlag)
    soundInit();

  autoFrameSkipLastTime = throttleLastTime = systemGetClock();

#ifdef AOS_SDL2
#ifdef __AMIGAOS4__
  char  w_title[512];
  strcpy(w_title,AMIGA_VERSION);
  /// dodac aos itp
  SDL_SetWindowTitle(window,w_title);
  if (!fullscreen)
     AmigaOS_ScreenTitle(window,filename);

#else
  SDL_SetWindowTitle(window,"VisualBoyAdvance");
#endif //AOS4
#else
  SDL_WM_SetCaption("VisualBoyAdvance", NULL);
#endif

  while(emulating) {
    if(!paused && active) {
      if(debugger && emulator.emuHasDebugger)
        dbgMain();
      else {
        emulator.emuMain(emulator.emuCount);
        if(rewindSaveNeeded && rewindMemory && emulator.emuWriteMemState) {
          rewindCount++;
          if(rewindCount > 8)
            rewindCount = 8;
          if(emulator.emuWriteMemState &&
             emulator.emuWriteMemState(&rewindMemory[rewindPos*REWIND_SIZE], 
                                       REWIND_SIZE)) {
            rewindPos = (rewindPos + 1) & 7;
            if(rewindCount == 8)
              rewindTopPos = (rewindTopPos + 1) & 7;
          }
        }

        rewindSaveNeeded = false;
      }
    } else {
      SDL_Delay(500);
    }
    sdlPollEvents();
    if(mouseCounter) {
      mouseCounter--;
      if(mouseCounter == 0)
        SDL_ShowCursor(SDL_DISABLE);
    }
  }


  emulating = 0;

  if (debug_fprintf) fprintf(stderr,"Shutting down\n");
  remoteCleanUp();
  soundShutdown();

  if(gbRom != NULL || rom != NULL) {
    sdlWriteBattery();
    emulator.emuCleanUp();
  }

  if(delta) {
    free(delta);
    delta = NULL;
  }

//begin free
  if(filterPix) {
    free(filterPix);
    filterPix = NULL;
 }

#ifdef AOS_SDL2
  // Close and destroy the window
  SDL_DestroyTexture(texture);
  SDL_FreeSurface(surface);
  SDL_DestroyRenderer(renderer);
  if(openGL)
    SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
#endif

#ifdef AOS_SDL2
// file drop go again
   if (dropped_file) goto run_again_dropfile;

   SDL_free(dropped_file);
#endif

  SDL_Quit();
  return 0;
}

void systemMessage(int num, const char *msg, ...)
{
  char buffer[SYSMSG_BUFFER_SIZE*2];
  va_list valist;
  
  va_start(valist, msg);
  vsprintf(buffer, msg, valist);

#ifdef __AMIGAOS4__
  EasyRequester(strdup((char*)buffer),
                       "OK");
#else
  if (debug_fprintf) fprintf(stderr, "%s\n", buffer);
#endif
  va_end(valist);
}

void systemDrawScreen()
{
  renderedFrames++;

  if(!openGL)
   SDL_LockSurface(surface);

  if(screenMessage) {
    if(cartridgeType == 1 && gbBorderOn) {
      gbSgbRenderBorder();
    }
    if(((systemGetClock() - screenMessageTime) < 3000) &&
       !disableStatusMessages) {
      drawText(pix, srcPitch, 10, srcHeight - 20,
               screenMessageBuffer); 
    } else {
      screenMessage = false;
    }
  }

  if(ifbFunction) {
    if(systemColorDepth == 16)
      ifbFunction(pix+destWidth+4, destWidth+4, srcWidth, srcHeight);
    else
      ifbFunction(pix+destWidth*2+4, destWidth*2+4, srcWidth, srcHeight);
  }

if (openGL) {
    int mult = 1;

    if(filterFunction) {
      int pitch = srcWidth * 4 + 4;

      filterFunction(pix + pitch,
                     pitch,
                     delta,
                     (u8*)filterPix,
                     srcWidth * 4 * 2,
                     srcWidth,
                     srcHeight);

      glPixelStorei(GL_UNPACK_ROW_LENGTH, srcWidth << 1);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, srcWidth << 1, srcHeight << 1,
                      GL_BGRA, GL_UNSIGNED_BYTE, filterPix);
      mult = 2;
    } else {
      glPixelStorei(GL_UNPACK_ROW_LENGTH, srcWidth + 1);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, srcWidth, srcHeight,
                      GL_BGRA, GL_UNSIGNED_BYTE, pix + ((srcWidth + 1) << 2));
    }

    glBegin(GL_TRIANGLE_STRIP);
      glTexCoord2f(0.0f, 0.0f);
      glVertex3i(0, 0, 0);
      glTexCoord2f(mult * srcWidth / (GLfloat) textureSize, 0.0f);
      glVertex3i(1, 0, 0);
      glTexCoord2f(0.0f, mult * srcHeight / (GLfloat) textureSize);
      glVertex3i(0, 1, 0);
      glTexCoord2f(mult * srcWidth / (GLfloat) textureSize,
                  mult * srcHeight / (GLfloat) textureSize);
      glVertex3i(1, 1, 0);
    glEnd();

#ifndef AOS_SDL2
   SDL_GL_SwapBuffers();
#else
 SDL_GL_SwapWindow(window);
#endif
  } else {
//no opengl
  if(filterFunction) {
    if(systemColorDepth == 16)
      filterFunction(pix+destWidth+4,destWidth+4, delta,
                     (u8*)surface->pixels,surface->pitch,
                     srcWidth,
                     srcHeight);
    else
      filterFunction(pix+destWidth*2+4,
                     destWidth*2+4,
                     delta,
                     (u8*)surface->pixels,
                     surface->pitch,
                     srcWidth,
                     srcHeight);
  } else {
    int destPitch = surface->pitch;
    u8 *src = pix;
    u8 *dest = (u8*)surface->pixels;
    int i;
    u32 *stretcher = (u32 *)sdlStretcher;
    if(systemColorDepth == 16)
      src += srcPitch;
    int option = sizeOption;
    if(yuv)
      option = 0;
    switch(sizeOption) {
    case 0:
      for(i = 0; i < srcHeight; i++) {
        SDL_CALL_STRETCHER;
        src += srcPitch;
        dest += destPitch;
      }
      break;
    case 1:
      for(i = 0; i < srcHeight; i++) {
        SDL_CALL_STRETCHER;     
        dest += destPitch;
        SDL_CALL_STRETCHER;
        src += srcPitch;
        dest += destPitch;
      }
      break;
    case 2:
      for(i = 0; i < srcHeight; i++) {
        SDL_CALL_STRETCHER;
        dest += destPitch;
        SDL_CALL_STRETCHER;
        dest += destPitch;
        SDL_CALL_STRETCHER;
        src += srcPitch;
        dest += destPitch;
      }
      break;
    case 3:
      for(i = 0; i < srcHeight; i++) {
        SDL_CALL_STRETCHER;
        dest += destPitch;
        SDL_CALL_STRETCHER;
        dest += destPitch;
        SDL_CALL_STRETCHER;
        dest += destPitch;
        SDL_CALL_STRETCHER;
        src += srcPitch;
        dest += destPitch;
      }
      break;
    }
  }

  if(showSpeed && fullscreen) {
    char buffer[50];
    if(showSpeed == 1)
      sprintf(buffer, "%d%%", systemSpeed);
    else
      sprintf(buffer, "%3d%%(%d, %d fps)", systemSpeed,
              systemFrameSkip,
              showRenderedFrames);
    if(showSpeedTransparent)
      drawTextTransp((u8*)surface->pixels,
                     surface->pitch,
                     10,
                     surface->h-20,
                     buffer);
    else
      drawText((u8*)surface->pixels,
               surface->pitch,
               10,
               surface->h-20,
               buffer);        
  }  

  SDL_UnlockSurface(surface);
//markus
#ifdef AOS_SDL2
#ifndef __AMIGAOS4__
  SDL_RenderClear(renderer);
#endif //AOS4
  SDL_UpdateTexture(texture, NULL, surface->pixels, surface->pitch);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
#else
  SDL_Flip(surface);
#endif
 } //end noopengl
}

bool systemReadJoypads()
{
  return true;
}

u32 systemReadJoypad(int which)
{
  if(which < 0 || which > 3)
    which = sdlDefaultJoypad;
  
  u32 res = 0;
  
  if(sdlButtons[which][KEY_BUTTON_A])
    res |= 1;
  if(sdlButtons[which][KEY_BUTTON_B])
    res |= 2;
  if(sdlButtons[which][KEY_BUTTON_SELECT])
    res |= 4;
  if(sdlButtons[which][KEY_BUTTON_START])
    res |= 8;
  if(sdlButtons[which][KEY_RIGHT])
    res |= 16;
  if(sdlButtons[which][KEY_LEFT])
    res |= 32;
  if(sdlButtons[which][KEY_UP])
    res |= 64;
  if(sdlButtons[which][KEY_DOWN])
    res |= 128;
  if(sdlButtons[which][KEY_BUTTON_R])
    res |= 256;
  if(sdlButtons[which][KEY_BUTTON_L])
    res |= 512;

  // disallow L+R or U+D of being pressed at the same time
  if((res & 48) == 48)
    res &= ~16;
  if((res & 192) == 192)
    res &= ~128;

  if(sdlButtons[which][KEY_BUTTON_SPEED])
    res |= 1024;
  if(sdlButtons[which][KEY_BUTTON_CAPTURE])
    res |= 2048;

  if(autoFire) {
    res &= (~autoFire);
    if(autoFireToggle)
      res |= autoFire;
    autoFireToggle = !autoFireToggle;
  }

  return res;
}

void systemSetTitle(const char *title)
{
#ifdef AOS_SDL2
  SDL_SetWindowTitle(window,title);
#ifdef __AMIGAOS4__
  AmigaOS_ScreenTitle(window,filename); //overwriting the WB string generated by SDL
#endif //AOS4
#else
  SDL_WM_SetCaption(title, NULL);
#endif
}

void systemShowSpeed(int speed)
{
  systemSpeed = speed;

  showRenderedFrames = renderedFrames;
  renderedFrames = 0;

  if(!fullscreen && showSpeed) {
    char buffer[80];
    if(showSpeed == 1)
#ifdef __AMIGAOS4__
      sprintf(buffer, AMIGA_VERSION"-%3d%%", systemSpeed);
#else
      sprintf(buffer, "VisualBoyAdvance-%3d%%", systemSpeed);
#endif
    else
#ifdef __AMIGAOS4__
      sprintf(buffer, AMIGA_VERSION"-%3d%%(%d, %d fps)", systemSpeed,
#else
      sprintf(buffer, "VisualBoyAdvance-%3d%%(%d, %d fps)", systemSpeed,
#endif
              systemFrameSkip,
              showRenderedFrames);

    systemSetTitle(buffer);
  }
}

void systemFrame()
{
}

void system10Frames(int rate)
{
  u32 time = systemGetClock();
  if(!wasPaused && autoFrameSkip && !throttle) {
    u32 diff = time - autoFrameSkipLastTime;
    int speed = 100;

    if(diff)
      speed = (1000000/rate)/diff;

    if(speed >= 98) {
      frameskipadjust++;

      if(frameskipadjust >= 3) {
        frameskipadjust=0;
        if(systemFrameSkip > 0)
          systemFrameSkip--;
      }
    } else {
      if(speed  < 80)
        frameskipadjust -= (90 - speed)/5;
      else if(systemFrameSkip < 9)
        frameskipadjust--;

      if(frameskipadjust <= -2) {
        frameskipadjust += 2;
        if(systemFrameSkip < 9)
          systemFrameSkip++;
      }
    }
  }
  if(!wasPaused && throttle) {
    if(!speedup) {
      u32 diff = time - throttleLastTime;
      
      int target = (1000000/(rate*throttle));
      int d = (target - diff);
      
      if(d > 0) {
        SDL_Delay(d);
      }
    }
    throttleLastTime = systemGetClock();
  }
  if(rewindMemory) {
    if(++rewindCounter >= rewindTimer) {
      rewindSaveNeeded = true;
      rewindCounter = 0;
    }
  }

  if(systemSaveUpdateCounter) {
    if(--systemSaveUpdateCounter <= SYSTEM_SAVE_NOT_UPDATED) {
      sdlWriteBattery();
      systemSaveUpdateCounter = SYSTEM_SAVE_NOT_UPDATED;
    }
  }

  wasPaused = false;
  autoFrameSkipLastTime = time;
}

void systemScreenCapture(int a)
{
  char buffer[2048];

  if(captureFormat) {
    if(captureDir[0])
      sprintf(buffer, "%s/%s%02d.bmp", captureDir, sdlGetFilename(filename), a);
    else if (homeDir)
#ifdef __AMIGAOS4__
      sprintf(buffer, "%s/%s/%s/%s%02d.bmp", homeDir, DOT_DIR, DOT_PIC, sdlGetFilename(filename), a);
#else
      sprintf(buffer, "%s/%s/%s%02d.bmp", homeDir, DOT_DIR, sdlGetFilename(filename), a);
#endif
    else
      sprintf(buffer, "%s%02d.bmp", filename, a);

    emulator.emuWriteBMP(buffer);
  } else {
    if(captureDir[0])
      sprintf(buffer, "%s/%s%02d.png", captureDir, sdlGetFilename(filename), a);
    else if (homeDir)
#ifdef __AMIGAOS4__
      sprintf(buffer, "%s/%s/%s/%s%02d.png", homeDir, DOT_DIR, DOT_PIC, sdlGetFilename(filename), a);
#else
      sprintf(buffer, "%s/%s/%s%02d.png", homeDir, DOT_DIR, sdlGetFilename(filename), a);
#endif
    else
      sprintf(buffer, "%s%02d.png", filename, a);
    emulator.emuWritePNG(buffer);
  }

  systemScreenMessage("Screen capture");
}

static void soundCallback(void *, u8 *stream, int len)
{
  if (len <= 0 || !emulating)
    return;
  SDL_mutexP(sdlSoundMutex);
  const int nAvail = soundBufferUsed();
  if (len > nAvail)
    len = nAvail;
  const int nAvail2 = ((sdlSoundTotalLen - sdlSoundRPos) + sdlSoundTotalLen) % sdlSoundTotalLen;
  if (len >= nAvail2) {
    memcpy(stream, &sdlSoundBuffer[sdlSoundRPos], nAvail2);
    sdlSoundRPos = 0;
    stream += nAvail2;
    len    -= nAvail2;
  }
  if (len > 0) {
    memcpy(stream, &sdlSoundBuffer[sdlSoundRPos], len);
    sdlSoundRPos = (sdlSoundRPos + len) % sdlSoundTotalLen;
    stream += len;
  }
  SDL_CondSignal(sdlSoundCond);
  SDL_mutexV(sdlSoundMutex);
}

void systemWriteDataToSoundBuffer()
{
  if (SDL_GetAudioStatus() != SDL_AUDIO_PLAYING)
  {
    SDL_PauseAudio(0);
  }

  int       remain = soundBufferLen;
  const u8 *wave   = reinterpret_cast<const u8 *>(soundFinalWave);
  if (remain <= 0)
    return;
  SDL_mutexP(sdlSoundMutex);
  int n;
  while (remain >= (n = soundBufferFree())) {
    const int nAvail = ((sdlSoundTotalLen - sdlSoundWPos) + sdlSoundTotalLen) % sdlSoundTotalLen;
    if (n >= nAvail) {
      memcpy(&sdlSoundBuffer[sdlSoundWPos], wave, nAvail);
      sdlSoundWPos  = 0;
      wave       += nAvail;
      remain     -= nAvail;
      n          -= nAvail;
    }
    if (n > 0) {
      memcpy(&sdlSoundBuffer[sdlSoundWPos], wave, n);
      sdlSoundWPos = (sdlSoundWPos + n) % sdlSoundTotalLen;
      wave   += n;
      remain -= n;
    }
    if (!emulating || speedup || throttle) {
      SDL_mutexV(sdlSoundMutex);
      return;
    }
    SDL_CondWait(sdlSoundCond, sdlSoundMutex);
  }
  const int nAvail = ((sdlSoundTotalLen - sdlSoundWPos) + sdlSoundTotalLen) % sdlSoundTotalLen;
  if (remain >= nAvail) {
    memcpy(&sdlSoundBuffer[sdlSoundWPos], wave, nAvail);
    sdlSoundWPos = 0;
    wave   += nAvail;
    remain -= nAvail;
  }
  if (remain > 0) {
    memcpy(&sdlSoundBuffer[sdlSoundWPos], wave, remain);
    sdlSoundWPos = (sdlSoundWPos + remain) % sdlSoundTotalLen;
  }
  SDL_mutexV(sdlSoundMutex);
}

bool systemSoundInit()
{
  SDL_AudioSpec audio;

  switch(soundQuality) {
  case 1:
    audio.freq = 44100;
    soundBufferLen = 1470*2;
    break;
  case 2:
    audio.freq = 22050;
    soundBufferLen = 736*2;
    break;
  case 4:
    audio.freq = 11025;
    soundBufferLen = 368*2;
    break;
  }
  audio.format=AUDIO_S16SYS;
  audio.channels = 2;
  audio.samples = 1024;
  audio.callback = soundCallback;
  audio.userdata = NULL;
  if(SDL_OpenAudio(&audio, NULL)) {
    if (debug_fprintf) fprintf(stderr,"Failed to open audio: %s\n", SDL_GetError());
    return false;
  }

  sdlSoundCond  = SDL_CreateCond();
  sdlSoundMutex = SDL_CreateMutex();

  soundBufferTotalLen = soundBufferLen * 10;
  sdlSoundRPos = sdlSoundWPos = 0;
  systemSoundOn = true;

  return true;
}

void systemSoundShutdown()
{
  SDL_mutexP(sdlSoundMutex);
  int iSave = emulating;
  emulating = 0;
  SDL_CondSignal(sdlSoundCond);
  SDL_mutexV(sdlSoundMutex);

  SDL_DestroyCond(sdlSoundCond);
  sdlSoundCond = NULL;

  SDL_DestroyMutex(sdlSoundMutex);
  sdlSoundMutex = NULL;

  SDL_CloseAudio();

  emulating = iSave;
  systemSoundOn = false;
}

void systemSoundPause()
{
  SDL_PauseAudio(1);
}

void systemSoundResume()
{
  SDL_PauseAudio(0);
}

void systemSoundReset()
{
}

u32 systemGetClock()
{
  return SDL_GetTicks();
}

void systemUpdateMotionSensor()
{
  if(sdlMotionButtons[KEY_LEFT]) {
    sensorX += 3;
    if(sensorX > 2197)
      sensorX = 2197;
    if(sensorX < 2047)
      sensorX = 2057;
  } else if(sdlMotionButtons[KEY_RIGHT]) {
    sensorX -= 3;
    if(sensorX < 1897)
      sensorX = 1897;
    if(sensorX > 2047)
      sensorX = 2037;
  } else if(sensorX > 2047) {
    sensorX -= 2;
    if(sensorX < 2047)
      sensorX = 2047;
  } else {
    sensorX += 2;
    if(sensorX > 2047)
      sensorX = 2047;
  }

  if(sdlMotionButtons[KEY_UP]) {
    sensorY += 3;
    if(sensorY > 2197)
      sensorY = 2197;
    if(sensorY < 2047)
      sensorY = 2057;
  } else if(sdlMotionButtons[KEY_DOWN]) {
    sensorY -= 3;
    if(sensorY < 1897)
      sensorY = 1897;
    if(sensorY > 2047)
      sensorY = 2037;
  } else if(sensorY > 2047) {
    sensorY -= 2;
    if(sensorY < 2047)
      sensorY = 2047;
  } else {
    sensorY += 2;
    if(sensorY > 2047)
      sensorY = 2047;
  }    
}

int systemGetSensorX()
{
  return sensorX;
}

int systemGetSensorY()
{
  return sensorY;
}

void systemGbPrint(u8 *data,int pages,int feed,int palette, int contrast)
{
}

void systemScreenMessage(const char *msg)
{
  screenMessage = true;
  screenMessageTime = systemGetClock();
  if(strlen(msg) > 20) {
    strncpy(screenMessageBuffer, msg, 20);
    screenMessageBuffer[20] = 0;
  } else
    strcpy(screenMessageBuffer, msg);  
}

bool systemCanChangeSoundQuality()
{
  return false;
}

bool systemPauseOnFrame()
{
  if(pauseNextFrame) {
    paused = true;
    pauseNextFrame = false;
    return true;
  }
  return false;
}

// Code donated by Niels Wagenaar (BoycottAdvance)

// GBA screensize.
#define GBA_WIDTH   240
#define GBA_HEIGHT  160

/* NOTE: These RGB conversion functions are not intended for speed,
   only as examples.
*/
inline void RGBtoYUV(Uint8 *rgb, int *yuv)
{
  yuv[0] = (int)((0.257 * rgb[0]) + (0.504 * rgb[1]) + (0.098 * rgb[2]) + 16);
  yuv[1] = (int)(128 - (0.148 * rgb[0]) - (0.291 * rgb[1]) + (0.439 * rgb[2]));
  yuv[2] = (int)(128 + (0.439 * rgb[0]) - (0.368 * rgb[1]) - (0.071 * rgb[2]));
}

void systemGbBorderOn()
{
  long flags;

  srcWidth = 256;
  srcHeight = 224;
  gbBorderLineSkip = 256;
  gbBorderColumnSkip = 48;
  gbBorderRowSkip = 40;

  destWidth = (sizeOption+1)*srcWidth;
  destHeight = (sizeOption+1)*srcHeight;

#ifdef AOS_SDL2
   flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_WINDOW_RESIZABLE;
   if(openGL) {
     flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;

     if (SDL_FULL)       window = SDL_CreateWindow("VBA", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, destWidth, destHeight, (flags|(fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0)));
        else window = SDL_CreateWindow("VBA", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, destWidth, destHeight, (flags|(fullscreen ? SDL_WINDOW_FULLSCREEN : 0)));

     context = SDL_GL_CreateContext(window);
     SDL_GL_SetSwapInterval(1);
     SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
     SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
     SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
     SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    } else {

     if (SDL_FULL) SDL_CreateWindowAndRenderer(destWidth, destHeight, (flags|(fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0)), &window, &renderer);
       else SDL_CreateWindowAndRenderer(destWidth, destHeight, (flags|(fullscreen ? SDL_WINDOW_FULLSCREEN : 0)), &window, &renderer);
   }

  SDL_SetWindowMinimumSize(window, destWidth, destHeight);
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

  surface = SDL_CreateRGBSurface(0, destWidth, destHeight, 32,
                                   0x00FF0000, 0x0000FF00,
                                   0x000000FF, 0xFF000000);
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                destWidth, destHeight);
  SDL_RenderClear(renderer); //markus renderer dont need on opengl ? fixme !!!

#else

  flags = SDL_ANYFORMAT | (fullscreen ? SDL_FULLSCREEN : 0);
 if(openGL) {
   flags |= SDL_OPENGL | SDL_RESIZABLE;
  } else
   flags |= SDL_HWSURFACE | SDL_DOUBLEBUF;

 surface = SDL_SetVideoMode(destWidth, destHeight, 16, flags);
 if(openGL)
  sdlOpenGLInit(destWidth, destHeight);
#endif

#ifndef C_CORE
  sdlMakeStretcher(srcWidth);
#else
  switch(systemColorDepth) {
  case 16:
    sdlStretcher = sdlStretcher16[sizeOption];
    break;
  case 24:
    sdlStretcher = sdlStretcher24[sizeOption];
    break;
  case 32:
    sdlStretcher = sdlStretcher32[sizeOption];
    break;
  default:
    if (debug_fprintf) fprintf(stderr, "Unsupported resolution: %d\n", systemColorDepth);
    exit(-1);
  }
#endif

  if(systemColorDepth == 16) {
    if(sdlCalculateMaskWidth(surface->format->Gmask) == 6) {
      Init_2xSaI(565);
      RGB_LOW_BITS_MASK = 0x821;
    } else {
      Init_2xSaI(555);
      RGB_LOW_BITS_MASK = 0x421;      
    }
    if(cartridgeType == 2) {
      for(int i = 0; i < 0x10000; i++) {
        systemColorMap16[i] = (((i >> 1) & 0x1f) << systemBlueShift) |
          (((i & 0x7c0) >> 6) << systemGreenShift) |
          (((i & 0xf800) >> 11) << systemRedShift);  
      }      
    } else {
      for(int i = 0; i < 0x10000; i++) {
        systemColorMap16[i] = ((i & 0x1f) << systemRedShift) |
          (((i & 0x3e0) >> 5) << systemGreenShift) |
          (((i & 0x7c00) >> 10) << systemBlueShift);  
      }
    }
    srcPitch = srcWidth * 2+4;
  } else {
    if(systemColorDepth != 32)
      filterFunction = NULL;
    RGB_LOW_BITS_MASK = 0x010101;
    if(systemColorDepth == 32) {
      Init_2xSaI(32);
    }
    for(int i = 0; i < 0x10000; i++) {
      systemColorMap32[i] = ((i & 0x1f) << systemRedShift) |
        (((i & 0x3e0) >> 5) << systemGreenShift) |
        (((i & 0x7c00) >> 10) << systemBlueShift);  
    }
    if(systemColorDepth == 32)
      srcPitch = srcWidth*4 + 4;
    else
      srcPitch = srcWidth*3;
  }
}
