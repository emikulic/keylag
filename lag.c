#include <err.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

static struct timespec MonoTime() {
  struct timespec t;
  if (clock_gettime(CLOCK_MONOTONIC, &t) == -1) err(1, "clock_gettime");
  return t;
}

static struct timespec Diff(struct timespec a, struct timespec b) {
  struct timespec out;
  out.tv_sec = a.tv_sec - b.tv_sec;
  out.tv_nsec = a.tv_nsec - b.tv_nsec;
  if (out.tv_nsec < 0) {
    out.tv_sec -= 1;
    out.tv_nsec += 1000000000;
  }
  return out;
}

static Atom intern_atom(Display* dpy, const char* atom_name) {
  Atom a = XInternAtom(dpy, atom_name, False);
  if (a == None) {
    errx(1, "XInternAtom(\"%s\") failed", atom_name);
  }
  return a;
}

int main() {
  Display* dpy = XOpenDisplay(NULL);
  if (!dpy) errx(1, "XOpenDisplay(\"%s\") failed", XDisplayName(NULL));

  int scr = DefaultScreen(dpy);
  unsigned long black = BlackPixel(dpy, scr);

  int width = 640;
  int height = 480;

  Window win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0,
                                   0,  // Position relative to parent.
                                   width, height,
                                   0,       // Border width.
                                   0,       // Border color.
                                   black);  // Background color.

  // Set window type to "utility" so i3wm will make it float. :^)
  {
    Atom atoms[2] = {intern_atom(dpy, "_NET_WM_WINDOW_TYPE_UTILITY"), None};
    XChangeProperty(dpy, win, intern_atom(dpy, "_NET_WM_WINDOW_TYPE"), XA_ATOM,
                    32, PropModeReplace, (unsigned char*)atoms, 1);
  }

  XSelectInput(
      dpy, win,
      StructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask);

  XMapWindow(dpy, win);
  XSync(dpy, False);

  int running = 1;
  struct timespec down = MonoTime();

  while (running) {
    XEvent e;
    XNextEvent(dpy, &e);
    switch (e.type) {
      case KeyPress: {
        down = MonoTime();
        KeySym ks = XkbKeycodeToKeysym(dpy, (KeyCode)e.xkey.keycode, 0, 0);
        // e.xkey.time seems to be measuring the same time (i.e. time of
        // XNextEvent, not time of actual input) except with less precision.
        if (ks == XK_Escape || ks == XK_q) running = 0;
        break;
      }

      case KeyRelease: {
        struct timespec up = MonoTime();
        struct timespec d = Diff(up, down);
        printf("%" PRId64 ".%09d\n", (uint64_t)d.tv_sec, (int)d.tv_nsec);
        fflush(stdout);
        break;
      }

      default:
        fprintf(stderr, "unhandled event, type = %d\n", e.type);
    }
  }

  XDestroyWindow(dpy, win);
  XCloseDisplay(dpy);
}

// vim:set ts=2 sw=2 tw=80 et:
