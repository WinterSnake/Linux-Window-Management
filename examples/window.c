/*
	Window Creation: Linux

	Written By: Ryan Smith
*/
#include <stdio.h>
#include <stdint.h>

// Window
#include <unistd.h>
#include <xcb/xcb.h>

// EGL
#include <EGL/egl.h>
#include <EGL/eglext.h>

// OpenGL
#include <dlfcn.h>
#define GL_VENDOR                         0x1F00
#define GL_RENDERER                       0x1F01
#define GL_VERSION                        0x1F02

int main(int argc, char** argv)
{
	(void)argc; (void)argv;

	// Connect to XServer
	int32_t screenCount;
	xcb_connection_t* const connection = xcb_connect(NULL, &screenCount);
	int32_t connectionError = xcb_connection_has_error(connection);
	if (connection == NULL || connectionError)
	{
		fprintf(stderr, "Unable to connect to XServer!\n");
		return 1;
	}
	// Get connection setup and screen info
	const xcb_setup_t* const setup = xcb_get_setup(connection);
	xcb_screen_iterator_t screenIter = xcb_setup_roots_iterator(setup);
	const xcb_screen_t* screen = screenIter.data;
	// Create window
	const xcb_window_t windowId = xcb_generate_id(connection);
	const uint32_t propName = XCB_CW_BACK_PIXEL;
	const uint32_t propValue = screen->white_pixel;

	const xcb_void_cookie_t createWindowRequest = xcb_create_window_checked(
		connection, XCB_COPY_FROM_PARENT, windowId, screen->root,  // XCB Connection, Depth, Window Id, Root
		0, 0, 1920, 1080, 0,  // Pos.X, Pos.Y, Width, Height, Border
		XCB_WINDOW_CLASS_INPUT_OUTPUT,
		screen->root_visual, propName, &propValue
	);
	xcb_generic_error_t* createWindowError = xcb_request_check(connection, createWindowRequest);
	if (createWindowError != NULL)
	{
		fprintf(stderr, "Unable to request window creation from XServer\n");
		return 1;
	}
	// Map Window
	const xcb_void_cookie_t mapWindowRequest = xcb_map_window_checked(connection, windowId);
	xcb_generic_error_t* mapWindowError = xcb_request_check(connection, mapWindowRequest);
	if (mapWindowError != NULL)
	{
		fprintf(stderr, "Unable to request map window from XServer\n");
		return 1;
	}

	// EGL: Initialize
	EGLDisplay eglDisplay = eglGetPlatformDisplay(
		EGL_PLATFORM_XCB_EXT, connection, (const EGLAttrib[]) {
			EGL_PLATFORM_XCB_SCREEN_EXT, screenCount, EGL_NONE
		}
	);
	if (eglDisplay == EGL_NO_DISPLAY)
	{
		fprintf(stderr, "Unable to get EGL display from XCB window\n");
		return 1;
	}
	EGLint major, minor;
	if (!eglInitialize(eglDisplay, &major, &minor))
	{
		fprintf(stderr, "Unable to initialize EGL over XCB window\n");
		return 1;
	}
	printf("[EGL::Log] Version: %i.%i\n", major, minor);
	printf("[EGL::Log] APIs: %s\n", eglQueryString(eglDisplay, EGL_CLIENT_APIS));
	printf("[EGL::Log] Vendor: %s\n", eglQueryString(eglDisplay, EGL_VENDOR));
	// EGL: Bind
	EGLBoolean bound = eglBindAPI(EGL_OPENGL_API);
	if (!bound)
	{
		fprintf(stderr, "Unable to bind EGL to OpenGL\n");
		return 1;
	}

	// EGL: Config
	EGLConfig eglConfig;
	EGLint configAttributes[] = {
		EGL_SURFACE_TYPE,      EGL_WINDOW_BIT,
		EGL_CONFORMANT,        EGL_OPENGL_BIT,
		EGL_RENDERABLE_TYPE,   EGL_OPENGL_BIT,
		EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,

		EGL_RED_SIZE,      8,
		EGL_GREEN_SIZE,    8,
		EGL_BLUE_SIZE,     8,
		EGL_DEPTH_SIZE,   24,
		EGL_STENCIL_SIZE,  8,
		EGL_NONE,
	};
	EGLint configCount;
	if (!eglChooseConfig(eglDisplay, configAttributes, &eglConfig, 1, &configCount))
	{
		fprintf(stderr, "Could not choose EGL config\n");
		return 1;
	}

	// EGL: Surface
	EGLint surfaceAttributes[] = {
            EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_LINEAR,
            EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
            EGL_NONE,
	};
	EGLSurface* eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, windowId, surfaceAttributes);
	if (eglSurface == EGL_NO_SURFACE)
	{
		fprintf(stderr, "Unable to create an EGL surface\n");
		return 1;
	}

	// EGL: Context
	EGLint contextAttributes[] = {
		EGL_CONTEXT_MAJOR_VERSION, 4,
		EGL_CONTEXT_MINOR_VERSION, 5,
		EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
		EGL_NONE,
	};
	EGLContext* eglContext = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, contextAttributes);
	if (eglContext == EGL_NO_CONTEXT)
	{
		fprintf(stderr, "Unable to create an OpenGL context\n");
		return 1;
	}
	EGLBoolean madeCurrent = eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
	if (!madeCurrent)
	{
		fprintf(stderr, "Unable to set OpenGL context\n");
		return 1;
	}

	// OpenGL: Load
	void* openGLHandle = dlopen("libGL.so.1", RTLD_NOW | RTLD_GLOBAL);
	if (!openGLHandle)
	{
		fprintf(stderr, "Unable to load GL library\n");
		return 1;
	}
	const char* (*glGetString)(unsigned int);
	glGetString = dlsym(openGLHandle, "glGetString");
	if (dlerror() != NULL)
	{
		fprintf(stderr, "Unable to get glGetString function\n");
		return 1;
	}
	dlclose(openGLHandle);
	printf("[GL::Log] Version: %s\n", glGetString(GL_VERSION));
	printf("[GL::Log] Vendor: %s\n", glGetString(GL_VENDOR));
	printf("[GL::Log] Renderer: %s\n", glGetString(GL_RENDERER));

	// Events
	sleep(10);

	// Close Window + Connection
	xcb_void_cookie_t destroyWindowRequest = xcb_destroy_window_checked(connection, windowId);
	xcb_generic_error_t* destoryWindowError = xcb_request_check(connection, destroyWindowRequest);
	if (destoryWindowError != NULL)
	{
		fprintf(stderr, "Unable to destroy window from XServer\n");
		return 1;
	}
	xcb_disconnect(connection);

	return 0;
}
