#include "common.h"
#include "actions.h"
#include "config.h"
#include "x11_hooks.h"
#include "time_hooks.h"
#include "vars.h"
#include <dlfcn.h>
#include <sys/types.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/XF86keysym.h>



Display *(*enhancer_real_XOpenDisplay)(const char *display)=NULL;
int (*enhancer_real_XMapWindow)(Display *display, Window win)=NULL;
int (*enhancer_real_XMapRaised)(Display *display, Window win)=NULL;
int (*enhancer_real_XRaiseWindow)(Display *display, Window win)=NULL;
int (*enhancer_real_XLowerWindow)(Display *display, Window win)=NULL;
Font (*enhancer_real_XLoadFont)(Display *display, const char *name)=NULL;
XFontStruct *(*enhancer_real_XLoadQueryFont)(Display *display, const char *name)=NULL;
int (*enhancer_real_XSendEvent)(Display *display, Window win, int propagate, long event_mask, void *event)=NULL;
int (*enhancer_real_XInternAtom)(Display *display, void *Atom, int Replace)=NULL;
const char *(*enhancer_real_XGetAtomName)(Display *display, Atom Atom)=NULL;
int (*enhancer_real_XSetCommand)(Display *display, Window *win, char *argv, int argc)=NULL;
int (*enhancer_real_XChangeWindowAttributes)(Display *disp, Window *win, int Mask, void *WinAttr);
int (*enhancer_real_XSetErrorHandler)(int (*handler)(Display *, XErrorEvent *))=NULL; 
int (*enhancer_real_XChangeProperty)(Display *display, Window w, Atom property, Atom type, int format, int mode, unsigned char *data, int nelements)=NULL; 
int (*enhancer_real_XNextEvent)(Display *display, XEvent *ev)=NULL; 


#define WINSTATE_DEL 0
#define WINSTATE_ADD 1
#define WINSTATE_TOGGLE 2

Atom ATOM_STRING, ATOM_WMCLASS, ATOM_WMNAME;
Display *g_Display=NULL;
Window g_MainWindow=NULL;

void X11SendSetStateEvent(Display *disp, Window Win, int AddOrDel, const char *StateStr)
{
    Atom StateAtom, StateValue;
    XEvent event;

		if (! enhancer_real_XInternAtom) return;
    StateAtom=enhancer_real_XInternAtom(disp, "_NET_WM_STATE", False);
    StateValue=enhancer_real_XInternAtom(disp, StateStr, False);


    memset( &event, 0, sizeof (XEvent) );
    event.xclient.type = ClientMessage;
    event.xclient.serial = 0;
    event.xclient.window = Win;
    event.xclient.message_type = StateAtom;
    event.xclient.send_event=True;
    event.xclient.format = 32;
    event.xclient.data.l[0] = AddOrDel;
    event.xclient.data.l[1] = StateValue;
    event.xclient.data.l[2] = 0;
    event.xclient.data.l[3] = 0;
    event.xclient.data.l[4] = 0;


    enhancer_real_XSendEvent(disp, DefaultRootWindow(disp), False, SubstructureRedirectMask | SubstructureNotifyMask, &event);
}



void X11SetWindowState(Display *disp, Window win, const char *StateStr)
{
XSetWindowAttributes WinAttr;

		if (strcmp(StateStr, "UNMANAGED")==0) 
		{
		memset(&WinAttr, 0, sizeof(XSetWindowAttributes));
		WinAttr.override_redirect=True;
		enhancer_real_XChangeWindowAttributes(disp, win, CWOverrideRedirect, &WinAttr);
		}
    else if (strcmp(StateStr,  "_NET_WM_STATE_ABOVE") ==0)
    {
        X11SendSetStateEvent(disp, win, WINSTATE_DEL, "_NET_WM_STATE_BELOW");
        X11SendSetStateEvent(disp, win, WINSTATE_ADD, StateStr);
    }
    else if (strcmp(StateStr,  "_NET_WM_STATE_BELOW") ==0)
    {
       X11SendSetStateEvent(disp, win, WINSTATE_DEL, "_NET_WM_STATE_ABOVE");
       X11SendSetStateEvent(disp, win, WINSTATE_ADD, StateStr);
			 enhancer_real_XLowerWindow(disp, win);
    }
		else if (strcmp(StateStr, "_NET_WM_STATE_ZORDER")==0)
		{
        X11SendSetStateEvent(disp, win, WINSTATE_DEL, "_NET_WM_STATE_ABOVE");
        X11SendSetStateEvent(disp, win, WINSTATE_DEL, "_NET_WM_STATE_BELOW");
		}
    else if (strcmp(StateStr,  "_NET_WM_STATE_SHADED") ==0)
    {
        X11SendSetStateEvent(disp, win, WINSTATE_ADD, "_NET_WM_STATE_SHADED");
    }
    else if (strcmp(StateStr,  "_NET_WM_STATE_UNSHADE") ==0)
    {
        X11SendSetStateEvent(disp, win, WINSTATE_DEL, "_NET_WM_STATE_SHADED");
    }
    else if (strcmp(StateStr,  "_NET_WM_STATE_FULLSCREEN") ==0)
    {
        X11SendSetStateEvent(disp, win, WINSTATE_ADD, "_NET_WM_STATE_FULLSCREEN");
    }
    else if (strcmp(StateStr,  "_NET_WM_STATE_ICONIZED") ==0)
    {
        X11SendSetStateEvent(disp, win, WINSTATE_ADD, "_NET_WM_STATE_HIDDEN");
    }
    else if (strcmp(StateStr,  "_NET_WM_STATE_RESTORED") ==0)
    {
        X11SendSetStateEvent(disp, win, WINSTATE_DEL, "_NET_WM_STATE_HIDDEN");
        X11SendSetStateEvent(disp, win, WINSTATE_DEL, "_NET_WM_STATE_FULLSCREEN");
    }
    else if (strcmp(StateStr,  "_NET_WM_STATE_NORMAL") ==0)
    {
        X11SendSetStateEvent(disp, win, WINSTATE_DEL, "_NET_WM_STATE_ABOVE");
        X11SendSetStateEvent(disp, win, WINSTATE_DEL, "_NET_WM_STATE_BELOW");
        X11SendSetStateEvent(disp, win, WINSTATE_DEL, "_NET_WM_STATE_SHADED");
        X11SendSetStateEvent(disp, win, WINSTATE_DEL, "_NET_WM_STATE_HIDDEN");
    }
    else X11SendSetStateEvent(disp, win, WINSTATE_ADD, StateStr);
}


int XMapWindow(Display *display, Window win)
{
int Flags, result;

Flags=enhancer_checkconfig_default(FUNC_XMapWindow, "XMapWindow", "", "", 0, 0);
if (Flags & FLAG_DENY) return(-1);
enhancer_real_XSetCommand(display, win, enhancer_argv, enhancer_argc);
result=enhancer_real_XMapWindow(display, win);

if (Flags & FLAG_X_STAY_ABOVE) X11SetWindowState(display, win, "_NET_WM_STATE_ABOVE");
if (Flags & FLAG_X_STAY_BELOW) X11SetWindowState(display, win, "_NET_WM_STATE_BELOW");
if (Flags & FLAG_X_ICONIZED) X11SetWindowState(display, win, "_NET_WM_STATE_ICONIZED");
if (Flags & FLAG_X_FULLSCREEN) X11SetWindowState(display, win, "_NET_WM_STATE_FULLSCREEN");
if (Flags & FLAG_X_NORMAL) X11SetWindowState(display, win, "_NET_WM_STATE_NORMAL");
//if (Flags & FLAG_X_TRANSPARENT) XSetWindowBackgroundPixmap(display, win, ParentRelative);
if (Flags & FLAG_X_UNMANAGED) X11SetWindowState(display, win, "UNMANAGED");

return(result);
}


int XMapRaised(Display *display, Window win)
{
int Flags, result;

Flags=enhancer_checkconfig_default(FUNC_XMapWindow, "XMapWindow", "", "", 0, 0);
if (Flags & FLAG_DENY) return(-1);
if (Flags & FLAG_X_STAY_ABOVE) X11SetWindowState(display, win, "_NET_WM_STATE_ABOVE");
if (Flags & FLAG_X_STAY_BELOW) X11SetWindowState(display, win, "_NET_WM_STATE_BELOW");
if (Flags & FLAG_X_ICONIZED) X11SetWindowState(display, win, "_NET_WM_STATE_ICONIZED");
if (Flags & FLAG_X_FULLSCREEN) X11SetWindowState(display, win, "_NET_WM_STATE_FULLSCREEN");
if (Flags & FLAG_X_NORMAL) X11SetWindowState(display, win, "_NET_WM_STATE_NORMAL");
//if (Flags & FLAG_X_TRANSPARENT) XSetWindowBackgroundPixmap(display, win, ParentRelative);
if (Flags & FLAG_X_UNMANAGED) X11SetWindowState(display, win, "UNMANAGED");


enhancer_real_XSetCommand(display, win, enhancer_argv, enhancer_argc);
result=enhancer_real_XMapRaised(display, win);
return(result);
}



int XRaiseWindow(Display *display, Window win)
{
int result;

result=enhancer_checkconfig_default(FUNC_XRaiseWindow, "XRaiseWindow", "", "", 0, 0);
if (result & FLAG_DENY) return(-1);

return(enhancer_real_XRaiseWindow(display, win));
}


int XLowerWindow(Display *display, Window win)
{
int result;

result=enhancer_checkconfig_default(FUNC_XLowerWindow, "XLowerWindow", "", "", 0, 0);
if (result & FLAG_DENY) return(-1);

return(enhancer_real_XLowerWindow(display, win));
}



XFontStruct *XLoadQueryFont(Display *display, const char *name)
{
char *font_list=NULL, *font_name=NULL;
const char *ptr;
XFontStruct *FS=NULL;
int Flags;

if (! enhancer_real_XLoadQueryFont) enhancer_x11_hooks();
font_list=enhancer_strcpy(font_list, name);
Flags=enhancer_checkconfig_exec_function(FUNC_XLoadFont, "XLoadFont", name, &font_list, NULL);

ptr=enhancer_strtok(font_list, ",", &font_name);
while (ptr)
{
FS=enhancer_real_XLoadQueryFont(display, font_name);
if (FS) break;
ptr=enhancer_strtok(ptr, ",", &font_name);
}

destroy(font_list);
destroy(font_name);
return(FS);
}


Font XLoadFont(Display *display, const char *name)
{
XFontStruct *FS;

FS=XLoadQueryFont(display, name);
if (FS) return(FS->fid);
return(BadFont);
}



int XChangeProperty(Display *display, Window w, Atom property, Atom type, int format, int mode, _Xconst unsigned char *data, int nelements)
{
int result;
unsigned char *Name=NULL, *Value=NULL, *Redirect=NULL;

Name=enhancer_strcpy(Name, enhancer_real_XGetAtomName(display, property));
if (type==ATOM_STRING) Value=enhancer_strncat(Value, data, nelements);
else Value=enhancer_strcpy(Value, "");

g_MainWindow=w;
enhancer_setvar(Name, Value);

Redirect=enhancer_strcpy(Redirect, Value);
result=enhancer_checkconfig_with_redirect(FUNC_XChangeProperty, "XChangeProperty", Name, Value, 0, 0, &Redirect);

if (type == ATOM_STRING) 
{
	result=enhancer_real_XChangeProperty(display, w, property, type, format, mode, Redirect, strlen(Redirect));
}
else result=enhancer_real_XChangeProperty(display, w, property, type, format, mode, data, nelements);

destroy(Redirect);
destroy(Name);
destroy(Value);

return(result); 
}

int X11SetProgName(Display *display, Window w, const char *Name)
{
return(enhancer_real_XChangeProperty(display, w, ATOM_WMNAME, ATOM_STRING, 8, PropModeReplace, (unsigned const char *) Name, strlen(Name)));
}


int X11SetProgClass(Display *display, Window w, const char *Name)
{
return(enhancer_real_XChangeProperty(display, w, ATOM_WMCLASS, ATOM_STRING, 8, PropModeReplace, (unsigned const char *) Name, strlen(Name)));
}


Display *XOpenDisplay(const char *DispID)
{
Display *disp;

if (! enhancer_real_XOpenDisplay)
{
enhancer_x11_hooks();
if (! enhancer_real_XOpenDisplay) printf("XOpenDisplay not mapped!\n");
}

disp=enhancer_real_XOpenDisplay(DispID);
g_Display=disp;

ATOM_STRING=enhancer_real_XInternAtom(disp, "STRING", 0);
ATOM_WMNAME=enhancer_real_XInternAtom(disp, "WM_NAME", 0);
ATOM_WMCLASS=enhancer_real_XInternAtom(disp, "WM_CLASS", 0);


return(disp);
}


/*

void X11Update()
{
time_t last=0, Now;
char *Tempstr=NULL;

Now=enhancer_gettime();
if (Now > last)
{
last=Now;
//Tempstr=enhancer_format_str(Tempstr, "%T $(WM_NAME)", "", "", "");
//if (g_MainWindow) X11SetProgName(g_Display, g_MainWindow, Tempstr);
//XSync(g_Display, 0);
destroy(Tempstr);
}
}


int XNextEvent(Display *display, XEvent *ev)
{
int result;

result=enhancer_real_XNextEvent(display, ev);
X11Update();
return(result);
}

int XSendEvent(Display *display, Window window, int propagate, long event_mask, void *event)
{
return(enhancer_real_XSendEvent(display, window, propagate, event_mask, event));
}
*/

int enhancer_X11ErrorHandler(Display *Disp, XErrorEvent *Event)
{
char *Tempstr=NULL;

Tempstr=realloc(Tempstr, 512);
//XGetErrorText(Disp, Event->error_code, Tempstr, 512);

//fprintf(stderr,"X11 Error: %s\n", Tempstr);

destroy(Tempstr);
return(FALSE);
}



void enhancer_x11_hooks()
{
void* handle = dlopen("libX11.so",RTLD_LAZY);

if (! handle) 
{
	handle=RTLD_NEXT;
}

if (! enhancer_real_XOpenDisplay) enhancer_real_XOpenDisplay = dlsym(handle, "XOpenDisplay");
if (! enhancer_real_XMapWindow) enhancer_real_XMapWindow = dlsym(handle, "XMapWindow");
if (! enhancer_real_XMapRaised) enhancer_real_XMapRaised = dlsym(handle, "XMapRaised");
if (! enhancer_real_XRaiseWindow) enhancer_real_XRaiseWindow = dlsym(handle, "XRaiseWindow");
if (! enhancer_real_XLowerWindow) enhancer_real_XLowerWindow = dlsym(handle, "XLowerWindow");
if (! enhancer_real_XLoadQueryFont) enhancer_real_XLoadQueryFont = dlsym(handle, "XLoadQueryFont");
if (! enhancer_real_XLoadFont) enhancer_real_XLoadFont = dlsym(handle, "XLoadFont");
if (! enhancer_real_XSendEvent) enhancer_real_XSendEvent = dlsym(handle, "XSendEvent");
if (! enhancer_real_XInternAtom) enhancer_real_XInternAtom = dlsym(handle, "XInternAtom");
if (! enhancer_real_XGetAtomName) enhancer_real_XGetAtomName = dlsym(handle, "XGetAtomName");
if (! enhancer_real_XSetCommand) enhancer_real_XSetCommand = dlsym(handle, "XSetCommand");
if (! enhancer_real_XChangeWindowAttributes) enhancer_real_XChangeWindowAttributes = dlsym(handle, "XChangeWindowAttributes");
if (! enhancer_real_XSetErrorHandler) enhancer_real_XSetErrorHandler = dlsym(handle, "XSetErrorHandler");
if (! enhancer_real_XChangeProperty) enhancer_real_XChangeProperty = dlsym(handle, "XChangeProperty");
if (! enhancer_real_XNextEvent) enhancer_real_XNextEvent = dlsym(handle, "XNextEvent");

enhancer_real_XSetErrorHandler(enhancer_X11ErrorHandler);
}

