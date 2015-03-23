#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#define WIN32_LEAN_AND_MEAN
#include <windows.h> 

#define null 0
#define assert(expression, message, ...) 				\
	do { 												\
		if (!(expression)) {							\
			fprintf(stderr, "/* ---- ASSERT ---- */\n");\
			fprintf(stderr,								\
				"LOCATION:   %s@%d\nCONDITION:  %s \n",	\
				__FILE__, __LINE__, #expression); 		\
			fprintf(stderr, "MESSAGE:    ");			\
			fprintf(stderr, message, ##__VA_ARGS__);	\
			fprintf(stderr, "\n");						\
			__builtin_trap();							\
		}												\
	} while (0);										\



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int iCmdshow)
{
	(void)hInstance;
	(void)hPrevInstance;
	(void)pScmdline;
	(void)iCmdshow;
	
	/*
	MSG msg;
	bool done = false;

	// Initialize the message structure.
	ZeroMemory(&msg, sizeof(MSG));
	
	// Loop until there is a quit message from the window or the user.
	while (!done)
	{
		// Handle the windows messages.
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// If windows signals to end the application then exit out.
		if(msg.message == WM_QUIT)
		{
			done = true;
		}
	}
	*/
}
