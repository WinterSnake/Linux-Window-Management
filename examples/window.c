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
		connection, XCB_COPY_FROM_PARENT, windowId, screen->root,
		0, 0, 1920, 1080, 1,
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
