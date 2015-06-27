/*
 * Copyright (C) 2002 Tugrul Galatali <tugrul@galatali.com>
 *
 * driver.c is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * driver.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/param.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "config.h"

#ifdef HAVE_GLEW
#include <GL/glew.h>
#include <GL/glxew.h>
#endif

#include <GL/gl.h>

#ifndef HAVE_GLEW
#include <GL/glx.h>
#endif

#ifdef HAVE_DPMS_EXT
#include <X11/extensions/dpms.h>
#endif

#include "driver.h"

#include "vroot.h"

xstuff_t *XStuff;

extern const char *hack_name;

/*
 * display parameters
 */
int rootWindow = False;
int glewInitialized = False;
#ifdef HAVE_GLEW
int frameTime = 10000;
int vsync = 1;
#else
int frameTime = 33333;
int vsync = 0;
#endif
int idleOnDPMS = 1;
int signalled = 0;

void createWindow (int argc, char **argv)
{
	XVisualInfo *visualInfo;
	GLXContext context;

	XStuff->screen_num = DefaultScreen (XStuff->display);
	XStuff->rootWindow = RootWindow (XStuff->display, XStuff->screen_num);

	if (rootWindow || XStuff->existingWindow) {
		XWindowAttributes gwa;
		Visual *visual;
		XVisualInfo templ;
		int outCount;

		XStuff->window = XStuff->existingWindow ? XStuff->existingWindow : XStuff->rootWindow;

		XGetWindowAttributes (XStuff->display, XStuff->window, &gwa);
		visual = gwa.visual;
		XStuff->windowWidth = gwa.width;
		XStuff->windowHeight = gwa.height;

		templ.screen = XStuff->screen_num;
		templ.visualid = XVisualIDFromVisual (visual);

		visualInfo = XGetVisualInfo (XStuff->display, VisualScreenMask | VisualIDMask, &templ, &outCount);

		if (!visualInfo) {
			fprintf (stderr, "%s: can't get GL visual for window 0x%lx.\n", XStuff->commandLineName, (unsigned long)XStuff->window);
			exit (1);
		}
	} else {
		int attributeList[] = {
			GLX_RGBA,
			GLX_RED_SIZE, 1,
			GLX_GREEN_SIZE, 1,
			GLX_BLUE_SIZE, 1,
			GLX_DEPTH_SIZE, 1,
			GLX_DOUBLEBUFFER,
			0
		};
		XSetWindowAttributes swa;
		XSizeHints hints;
		XWMHints wmHints;

		visualInfo = NULL;

		if (!(visualInfo = glXChooseVisual (XStuff->display, XStuff->screen_num, attributeList))) {
			fprintf (stderr, "%s: can't open GL visual.\n", XStuff->commandLineName);
			exit (1);
		}

		swa.colormap = XCreateColormap (XStuff->display, XStuff->rootWindow, visualInfo->visual, AllocNone);
		swa.border_pixel = swa.background_pixel = swa.backing_pixel = BlackPixel (XStuff->display, XStuff->screen_num);
		swa.event_mask = KeyPressMask | StructureNotifyMask;

		XStuff->windowWidth = DisplayWidth (XStuff->display, XStuff->screen_num) / 2;
		XStuff->windowHeight = DisplayHeight (XStuff->display, XStuff->screen_num) / 2;

		XStuff->window =
			XCreateWindow (XStuff->display, XStuff->rootWindow, 0, 0, XStuff->windowWidth, XStuff->windowHeight, 0, visualInfo->depth, InputOutput, visualInfo->visual,
				       CWBorderPixel | CWBackPixel | CWBackingPixel | CWColormap | CWEventMask, &swa);

		hints.flags = USSize;
		hints.width = XStuff->windowWidth;
		hints.height = XStuff->windowHeight;

		wmHints.flags = InputHint;
		wmHints.input = True;

		XmbSetWMProperties (XStuff->display, XStuff->window, hack_name, hack_name, argv, argc, &hints, &wmHints, NULL);
	}

	context = glXCreateContext (XStuff->display, visualInfo, 0, GL_TRUE);
	if (!context) {
		fprintf (stderr, "%s: can't open GLX context.\n", XStuff->commandLineName);
		exit (1);
	}

	if (!glXMakeCurrent (XStuff->display, XStuff->window, context)) {
		fprintf (stderr, "%s: can't set GL context.\n", XStuff->commandLineName);
		exit (1);
	}

	XFree (visualInfo);
	XMapWindow (XStuff->display, XStuff->window);
}

void clearBuffers() {
	int i;
	XEvent event;

	for (i = 0; i < 4; i++) {
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glXSwapBuffers (XStuff->display, XStuff->window);

		while (XPending (XStuff->display)) {
			XNextEvent (XStuff->display, &event);
		}
	}
}

int deltaus(const struct timeval now, const struct timeval then) {
	return (now.tv_sec - then.tv_sec) * 1000000 + now.tv_usec - then.tv_usec;
}

void mainLoop (void)
{
	int bFPS = False;
	XEvent event;
	Atom XA_WM_PROTOCOLS = XInternAtom (XStuff->display, "WM_PROTOCOLS", False);
	Atom XA_WM_DELETE_WINDOW = XInternAtom (XStuff->display, "WM_DELETE_WINDOW", False);
	struct timeval cycleStart, now, fps_time;
	int fps = 0;
	int drawEnabled = 1;

	if (!rootWindow) {
		XSetWMProtocols (XStuff->display, XStuff->window, &XA_WM_DELETE_WINDOW, 1);
	}

	clearBuffers ();

#ifdef HAVE_GLEW
	if (glewInitialized) {
		if ((vsync > 0) && GLXEW_SGI_swap_control) {
			glXSwapIntervalSGI (vsync);
		}
	}
#endif

#ifdef HAVE_DPMS_EXT
	int dpmsAvailable = 0;
	{
		int event_number, error_number;
		if (DPMSQueryExtension(XStuff->display, &event_number, &error_number)) {
			if (DPMSCapable(XStuff->display)) {
				dpmsAvailable = 1;
			}
		}
	}
#endif

	gettimeofday (&cycleStart, NULL);
	now = fps_time = cycleStart;
	int frameTimeSoFar = 0;
#ifdef HAVE_DPMS_EXT
	int sinceLastDPMSPoll = 0;
#endif
	while (!signalled) {
#ifdef HAVE_DPMS_EXT
		sinceLastDPMSPoll = sinceLastDPMSPoll + frameTimeSoFar;
		if (idleOnDPMS && dpmsAvailable && (sinceLastDPMSPoll > 1000000)) {
			CARD16 state;
			BOOL onoff;

			sinceLastDPMSPoll = 0;
			drawEnabled = 1;

			if (DPMSInfo(XStuff->display, &state, &onoff)) {
				if (onoff && (state != DPMSModeOn)) drawEnabled = 0;
			}

			// If the display isn't on, kick back and poll DPMS every second
			if (!drawEnabled) {
				sleep(1);
			}
		}
#endif

		if (drawEnabled) {
			hack_draw (XStuff, (double)now.tv_sec + now.tv_usec / 1000000.0f, frameTimeSoFar / 1000000.0f);

			glXSwapBuffers (XStuff->display, XStuff->window);
		}

		if (bFPS) {
			if (fps != -1)
				fps++;

			if (now.tv_sec > fps_time.tv_sec) {
				if (fps != -1) {
					printf ("%.4f fps\n", fps / (deltaus(now, fps_time) / 1000000.0));
				}

				fps = 0;
				fps_time.tv_sec = now.tv_sec;
				fps_time.tv_usec = now.tv_usec;
			}
		}

		while (XPending (XStuff->display)) {
			KeySym keysym;
			char c = 0;

			XNextEvent (XStuff->display, &event);
			switch (event.type) {
			case ConfigureNotify:
				if ((int)XStuff->windowWidth != event.xconfigure.width || (int)XStuff->windowHeight != event.xconfigure.height) {
					XStuff->windowWidth = event.xconfigure.width;
					XStuff->windowHeight = event.xconfigure.height;

					clearBuffers ();

					hack_reshape (XStuff);
				}

				break;
			case KeyPress:
				XLookupString (&event.xkey, &c, 1, &keysym, 0);

				if (c == 'f') {
					bFPS = !bFPS;

					if (bFPS) {
						fps = -1;
						gettimeofday (&fps_time, NULL);
					}
				}

				if (c == 'q' || c == 'Q' || c == 3 || c == 27)
					return;

				break;
			case ClientMessage:
				if (event.xclient.message_type == XA_WM_PROTOCOLS) {
					if (event.xclient.data.l[0] == (int)XA_WM_DELETE_WINDOW) {
						return;
					}
				}
				break;
			case DestroyNotify:
				return;
			}
		}

		gettimeofday (&now, NULL);
		frameTimeSoFar = deltaus(now, cycleStart);

		if (frameTime) {
			while (frameTimeSoFar < frameTime) {
#ifdef HAVE_NANOSLEEP
				struct timespec hundreth;

				hundreth.tv_sec = 0;
				hundreth.tv_nsec = (frameTime - frameTimeSoFar) * 1000;

				nanosleep (&hundreth, NULL);
#else
				usleep (frameTime - frameTimeSoFar);
#endif

				gettimeofday (&now, NULL);
				frameTimeSoFar = deltaus(now, cycleStart);
			}

			int delta = 0;
			do {
				cycleStart.tv_usec += frameTime;
				if (cycleStart.tv_usec > 1000000) {
					cycleStart.tv_sec++;
					cycleStart.tv_usec -= 1000000;
				}
				delta = deltaus(now, cycleStart);
			} while (delta > frameTime);
		} else {
			cycleStart = now;
		}
	}
}

void handle_global_opts (int c)
{
	switch (c) {
	case 'r':
		rootWindow = 1;

		break;
	case 'x':
		c = strtol_minmaxdef (optarg, 10, 0, 10000, 1, 100, "--maxfps: ");

		frameTime = (c > 0) ? 1000000 / c : 0;

		break;
	case 'y':
		vsync = strtol_minmaxdef (optarg, 10, 0, 100, 1, 2, "--vsync: ");

#ifndef HAVE_GLEW
		if (vsync)
			fprintf (stderr, "Not compiled with GLEW, vsync not supported.\n");
#endif

		break;
	case 'M':
		idleOnDPMS = strtol_minmaxdef (optarg, 10, 0, 1, 0, 1, "--dpms: ");

#ifndef HAVE_DPMS_EXT
		if (idleOnDPMS)
			fprintf (stderr, "Not compiled with DPMS, idling on DPMS not supported.\n");
#endif

		break;
	}
}

int strtol_minmaxdef (const char *optarg, const int base, const int min, const int max, const int type, const int def, const char *errmsg)
{
	const int result = strtol (optarg, (char **)NULL, base);

	if (result < min) {
		if (errmsg) {
			fprintf (stderr, "%s %d < %d, using %d instead.\n", errmsg, result, min, type ? min : def);
		}

		return type ? min : def;
	}

	if (result > max) {
		if (errmsg) {
			fprintf (stderr, "%s %d > %d, using %d instead.\n", errmsg, result, max, type ? max : def);
		}

		return type ? max : def;
	}

	return result;
}

void signalHandler (int sig)
{
	signalled = 1;
}

int main (int argc, char *argv[])
{
	struct sigaction sa;
	char *display_name = NULL;	/* Server to connect to */
	int i, j;

	XStuff = (xstuff_t *) malloc (sizeof (xstuff_t));
	XStuff->commandLineName = argv[0];

	srandom ((unsigned)time (NULL));

	XStuff->existingWindow = 0;
	for (i = 0; i < argc; i++) {
		if (!strcmp (argv[i], "-window-id")) {
			if ((argv[i + 1][0] == '0') && ((argv[i + 1][1] == 'x') || (argv[i + 1][1] == 'X'))) {
				XStuff->existingWindow = strtol ((char *)(argv[i + 1] + 2), (char **)NULL, 16);
			} else {
				XStuff->existingWindow = strtol ((char *)(argv[i + 1]), (char **)NULL, 10);
			}

			for (j = i + 2; j < argc; j++) {
				argv[j - 2] = argv[j];
			}

			argc -= 2;

			break;
		}
	}

	hack_handle_opts (argc, argv);

	XStuff->display = NULL;
	XStuff->window = 0;

	memset ((void *)&sa, 0, sizeof (struct sigaction));
	sa.sa_handler = signalHandler;
	sigaction (SIGINT, &sa, 0);
	sigaction (SIGPIPE, &sa, 0);
	sigaction (SIGQUIT, &sa, 0);
	sigaction (SIGTERM, &sa, 0);

	/*
	 * Connect to the X server.
	 */
	if (NULL == (XStuff->display = XOpenDisplay (display_name))) {
		fprintf (stderr, "%s: can't connect to X server %s\n", XStuff->commandLineName, XDisplayName (display_name));
		exit (1);
	}

	createWindow (argc, argv);

#ifdef HAVE_GLEW
	if (glewInit () == GLEW_OK) {
		glewInitialized = True;
	}
#endif

	hack_init (XStuff);

	mainLoop ();

	/*
	 * Clean up.
	 */
	if (XStuff->display) {
		if (XStuff->window) {
			hack_cleanup (XStuff);

			if (!((rootWindow) || (XStuff->existingWindow)))
				XDestroyWindow (XStuff->display, XStuff->window);
		}

		XCloseDisplay (XStuff->display);
	}

	return 0;
}
