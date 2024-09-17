/*
	Window Creation: Linux

	Written By: Ryan Smith
*/
#include <stdio.h>
#include <stdint.h>

#include <unistd.h>
#include <xcb/xcb.h>

int main(int argc, char** argv)
{
	(void)argc; (void)argv;

	// Connect to XServer
	int32_t screenId;
	xcb_connection_t* const connection = xcb_connect(NULL, &screenId);
	if (xcb_connection_has_error(connection))
	{
		fprintf(stderr, "Unable to connect to display server!\n");
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

	xcb_create_window(
		connection, XCB_COPY_FROM_PARENT, windowId, screen->root,
		0, 0, 1920, 1080, 1,
		XCB_WINDOW_CLASS_INPUT_OUTPUT,
		screen->root_visual, propName, &propValue
	);
	// Map Window
	const xcb_void_cookie_t mapWindowRequest = xcb_map_window(connection, windowId);
	xcb_flush(connection);

	// Events
	sleep(10);

	xcb_disconnect(connection);

	return 0;
}
