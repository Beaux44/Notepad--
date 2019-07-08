// windres -i Rsrcs/icons.rc -o Rsrcs/icons.o
// debug (there's not actually any debugging "features" (printfs), but they can be added as needed vOv)
// gcc NPP1.c -o NPP1.exe -std=c11 -O3 -Xlinker Libs/gdi32.lib Rsrcs/icons.o

// I notice that somehow or another, compiling like this instantly speeds up all of the text editing,
// so don't worry too much about the "debug" mode being slow
// release
// gcc NPP1.c -o NPP1.exe -std=c11 -O3 -Xlinker Libs/gdi32.lib Rsrcs/icons.o -Wl,-subsystem,windows

// #define UNICODE
// #define _UNICODE
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "Headers/argv.h"
#include "Rsrcs/rsrcs.h"
#define MOD_NOREPEAT     0x4000
#define IDM_FILE_NEW     1
#define IDM_FILE_OPEN    2
#define IDM_FILE_EXIT    3
#define IDM_FILE_SAVE    4
#define IDM_FILE_SAVE_AS 5
#define IDM_EDIT_FIND    6
#define IDM_EDIT_FNEXT   7
#define IDM_EDIT_REPL    8
#define IDM_EDIT_GOTO    9
#define IDM_EDIT_SALL    10
#define IDM_EDIT_TIME    11
#define IDM_FORM_WORDW   12
#define HOT_SAVE         13
#define failedSave()     MessageBox(hWnd, "Failed to save file", "Warning", MB_OK | MB_ICONWARNING)

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void addMenu(HWND);
_Bool saveFile();
HINSTANCE hInst;
HWND hTextbox, hWnd;
HMENU menuBar, fileMenu, editMenu, formMenu;
char *fileName = NULL;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char *lpCmdLine, int nCmdShow) {
  char **argv    = NULL;
  int argc       = 0;
  hInst          = hInstance;
  if(*lpCmdLine != '\0')
    argv = CommandLineToArgvA(GetCommandLineA(), &argc);

  const char *const title = "winhello";
  MSG                msg;
  NONCLIENTMETRICS   ncm;
  WNDCLASSEX         wnd;

  ncm.cbSize        = sizeof(ncm);
  wnd.cbSize        = sizeof(wnd);
  wnd.style         = CS_HREDRAW | CS_VREDRAW;
  wnd.lpfnWndProc   = WndProc;
  wnd.cbClsExtra    = 0;
  wnd.cbWndExtra    = 0;
  wnd.hInstance     = hInst;
  wnd.hIcon         = LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, 256, 256, LR_DEFAULTCOLOR);
  wnd.hIconSm       = LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON_SM), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
  wnd.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wnd.hbrBackground = GetStockObject(BLACK_BRUSH);
  wnd.lpszClassName = title;
  wnd.lpszMenuName  = NULL;

  RegisterClassEx(&wnd);
  hWnd = CreateWindow(title, "Notepad--", WS_TILEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                      CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, NULL);


  // Reads file into memory
  char *buffer = NULL;
  if(argc > 1) {
    fileName = argv[1];
    long length;
    FILE *fp = fopen(fileName, "rb");

    if(fp) {
      fseek(fp, 0, SEEK_END);
      length = ftell(fp);
      if(length) {
        fseek(fp, 0, SEEK_SET);
        buffer = malloc(length * sizeof(char));
        if(buffer)
          fread(buffer, 1, length, fp);
        buffer[length] = '\0';
        fclose(fp);
      }
    }
  }

  hTextbox = CreateWindow("edit", buffer ? buffer : "", ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | WS_CHILD, 0, 0, 300, 300, hWnd, NULL, hInst, NULL);
  free(buffer);
  RegisterHotKey(hWnd, HOT_SAVE, MOD_CONTROL | MOD_NOREPEAT, 'S'); // This makes a global hotkey -- it will block any other processes from using Ctrl+S
                                                                   // as a hotkey, I need to figure out how to make it not global :v

  SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
  const HFONT hNewFont = CreateFont(15, 10, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                    CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, "Courier New");
  SendMessage(hTextbox, WM_SETFONT, (WPARAM)hNewFont, 0);
  SendMessage(hTextbox, EM_LIMITTEXT, 0, 0);

  SetClassLong(hWnd, GCL_HICON, (long) wnd.hIcon);
  SetClassLong(hWnd, GCL_HICONSM, (long) wnd.hIconSm);

  ShowWindow(hWnd, nCmdShow);
  UpdateWindow(hWnd);

  while(GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
  long style;
  unsigned int state;
  switch(iMsg) {
    case WM_CREATE:
      addMenu(hWnd);
      return 0L;
    case WM_COMMAND:
      switch(LOWORD(wParam)) {
        case IDM_FILE_NEW:
        case IDM_FILE_OPEN:
        case IDM_FILE_SAVE_AS:
          MessageBeep(MB_ICONINFORMATION);
          break;
        case IDM_FILE_SAVE:
          if(!saveFile()) failedSave();
          return 0;
        case IDM_FILE_EXIT:
          SendMessage(hWnd, WM_CLOSE, 0, 0);
          break;
        case IDM_FORM_WORDW: // Need to destroy edit control and create new one with/without word wrap
          state = GetMenuState(formMenu, IDM_FORM_WORDW, MF_BYCOMMAND);
          style = GetWindowLong(hTextbox, GWL_STYLE);
          if(state == MF_CHECKED) {
            style |= ES_AUTOHSCROLL;
            style |= WS_HSCROLL;
            SetWindowLong(hTextbox, GWL_STYLE, style);
            SetWindowPos(hTextbox, 0, 0, 0, 0, 0, SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_DRAWFRAME);
            ShowWindow(hTextbox, SW_SHOW);
            CheckMenuItem(formMenu, IDM_FORM_WORDW, MF_UNCHECKED);
          } else {
            style &= ~ES_AUTOHSCROLL;
            style &= ~WS_HSCROLL;
            SetWindowLong(hTextbox, GWL_STYLE, style);
            SetWindowPos(hTextbox, 0, 0, 0, 0, 0, SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_DRAWFRAME);
            ShowWindow(hTextbox, SW_SHOW);
            CheckMenuItem(formMenu, IDM_FORM_WORDW, MF_CHECKED);
          }
          break;
      }
      return 0L;
    case WM_HOTKEY:
      switch(LOWORD(wParam)) {
        case HOT_SAVE:
          if(!saveFile()) failedSave();
      }
      return 0L;
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0L;
    case WM_CTLCOLOREDIT:
      SetTextColor((HDC)wParam, RGB(255, 255, 255));
    case WM_CTLCOLORSCROLLBAR:
      SetBkColor((HDC)wParam, RGB(0, 0, 0));
      return (LRESULT) GetStockObject(BLACK_BRUSH);
    case WM_SIZE:
      MoveWindow(hTextbox, 0, 0, LOWORD(lParam), HIWORD(lParam), 1);
      ShowWindow(hTextbox, SW_SHOWNORMAL);
      return 0L;
  }

  return DefWindowProc(hWnd, iMsg, wParam, lParam);
}

void addMenu(HWND hWnd) {
  menuBar     = CreateMenu(),
    fileMenu  = CreateMenu(),
    editMenu  = CreateMenu(),
    formMenu  = CreateMenu();

  AppendMenuA(fileMenu, MF_STRING, IDM_FILE_NEW,     "&New          Ctrl+N");
  AppendMenuA(fileMenu, MF_STRING, IDM_FILE_OPEN,    "&Open         Ctrl+O");
  AppendMenuA(fileMenu, MF_STRING, IDM_FILE_SAVE,    "&Save         Ctrl+S");
  AppendMenuA(fileMenu, MF_STRING, IDM_FILE_SAVE_AS, "Save &As...   Ctrl+Shift+S");
  AppendMenuA(fileMenu, MF_SEPARATOR, 0, NULL);
  AppendMenuA(fileMenu, MF_STRING, IDM_FILE_EXIT,    "E&xit         Alt-F4");

  AppendMenuA(editMenu, MF_STRING, IDM_EDIT_FIND,  "&Find        Ctrl-F");
  AppendMenuA(editMenu, MF_STRING, IDM_EDIT_FNEXT, "Find &Next   F3");
  AppendMenuA(editMenu, MF_STRING, IDM_EDIT_REPL,  "&Replace     Ctrl-H");
  AppendMenuA(editMenu, MF_STRING, IDM_EDIT_GOTO,  "&Go to...    Ctrl-G");
  AppendMenuA(editMenu, MF_SEPARATOR, 0, NULL);
  AppendMenuA(editMenu, MF_STRING, IDM_EDIT_SALL,  "Select &All  Ctrl-A");
  AppendMenuA(editMenu, MF_STRING, IDM_EDIT_TIME,  "Time/&Date   F5");

  AppendMenuA(formMenu, MF_STRING, IDM_FORM_WORDW, "&Word Wrap");
  CheckMenuItem(formMenu, IDM_FORM_WORDW, MF_CHECKED);

  AppendMenuA(menuBar, MF_POPUP, (unsigned int) fileMenu, "&File");
  AppendMenuA(menuBar, MF_POPUP, (unsigned int) editMenu, "&Edit");
  AppendMenuA(menuBar, MF_POPUP, (unsigned int) formMenu, "F&ormat");

  // MENUINFO mi = { 0 };
  // mi.cbSize = sizeof(mi);
  // mi.fMask = MIM_BACKGROUND|MIM_APPLYTOSUBMENUS;
  // mi.hbrBack = GetStockObject(BLACK_BRUSH);
  // SetMenuInfo(menuBar, &mi);  // Ugly as shit; need to figure out something else

  SetMenu(hWnd, menuBar);
}

_Bool saveFile() {
  if(fileName) {
    FILE *fp = fopen(fileName, "wb+");
    if(fp) {
      unsigned int chars = 1024*1024;
      char *buffer = malloc(chars), *tmp = buffer;
      while(buffer && GetWindowText(hTextbox, buffer, chars) >= chars - 1 && chars)
        if((tmp = realloc(buffer, chars <<= 1)))
          buffer = tmp;
        else
          break;

      if(tmp && buffer) {
        fputs(buffer, fp);
        fclose(fp);
        free(buffer);
        return 1;
      } else
        free(buffer);
    }
  }
  return 0;
}
