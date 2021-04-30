
void SPrint(char* buf, int buf_size, const char *str, ...)
{
	va_list ptr;
	va_start(ptr,str);
	vsprintf_s(buf,buf_size,str,ptr);
	va_end(ptr);
}

#ifdef _DEBUG

#define Assert(expression, message, ...) 			\
do {												\
	__pragma(warning(suppress:4127))				\
	if (!(expression)) {							\
		char __buf[512];							\
		SPrint(__buf, 512,							\
			"/* ---- Renderland Assert ---- */ \n"	\
			"LOCATION:  %s@%d		\n"				\
			"CONDITION:  %s			\n"				\
			"MESSAGE: " message "	\n",			\
			__FILE__, __LINE__, 					\
			#expression,							\
			##__VA_ARGS__);							\
		printf("%s\n",__buf);						\
		if (IsDebuggerPresent())					\
		{											\
			OutputDebugString(__buf);				\
			OutputDebugString("\n");				\
			DebugBreak();							\
		}											\
		else										\
		{											\
			MessageBoxA(NULL, 						\
				__buf,								\
				"Assert Failed", 					\
				MB_ICONERROR | MB_OK);				\
			exit(-1);								\
		}											\
	}												\
	__pragma(warning(default:4127))					\
} while (0);										\

#define Unimplemented()								\
do {												\
	char __buf[512];								\
	SPrint(__buf, 512,								\
		"/* ---- Renderland Assert ---- */ \n"		\
		"LOCATION:  %s@%d		\n"					\
		"MESSAGE: This code is unimplemented!\n",	\
		__FILE__, __LINE__);						\
	printf("%s\n",__buf);							\
	if (IsDebuggerPresent())						\
	{												\
		OutputDebugString(__buf);					\
		OutputDebugString("\n");					\
		DebugBreak();								\
	}												\
	else											\
	{												\
		MessageBoxA(NULL, 							\
			__buf,									\
			"Assert Failed", 						\
			MB_ICONERROR | MB_OK);					\
		exit(-1);									\
	}												\
} while (0);										\


#else // ifdef _DEBUG

#define Assert(expression, message, ...)
#define Unimplemented()

#endif // ifdef _DEBUG

#define Prompt(message, ...) 						\
do { 												\
	char __buf[512];								\
	SPrint(__buf, 512,								\
		"/* ---- Renderland Prompt ---- */ \n"		\
		"LOCATION:  %s@%d		\n"					\
		"MESSAGE: " message "	\n",				\
		__FILE__, __LINE__, 						\
		##__VA_ARGS__);								\
	{												\
		MessageBoxA(NULL, 							\
			__buf,									\
			"Prompt", 								\
			MB_OK);									\
	}												\
} while (0);										\
