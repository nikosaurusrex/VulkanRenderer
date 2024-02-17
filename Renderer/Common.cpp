#include "Common.h"

static void ColorReset(FILE *stream) {
	printf("\x1b[0m");
}

void LogFatal(const char *format, ...) {
    va_list args;
    va_start(args, format);
    
	printf("\x1b[31m[Fatal] ");

    vfprintf(stderr, format, args);
    putc('\n', stderr);
    va_end(args);

	ColorReset(stderr);
    exit(1);
}

void LogError(const char *format, ...) {
    va_list args;
    va_start(args, format);
    
	printf("\x1b[31m[Error] ");

    vfprintf(stderr, format, args);
    putc('\n', stderr);
    va_end(args);

	ColorReset(stderr);
}

void LogDev(const char *format, ...) {
    va_list args;
    va_start(args, format);
    
	printf("\x1b[33m");

    vfprintf(stdout, format, args);
    putc('\n', stdout);
    va_end(args);

	ColorReset(stdout);
}

void LogInfo(const char *format, ...) {
    va_list args;
    va_start(args, format);
    
	printf("\x1b[37m");

    vfprintf(stdout, format, args);
    putc('\n', stdout);
    va_end(args);

	ColorReset(stdout);
}

char *ReadEntireFile(const char *file_path) {
    FILE *file = fopen(file_path, "rb");
    if (!file) return 0;

    fseek(file, 0, SEEK_END);
    auto length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *data = new char[length + 1];

    fread(data, 1, length, file);
    data[length] = 0;

    fclose(file);

    return data;
}
