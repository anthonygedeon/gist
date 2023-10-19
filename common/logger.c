#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

#include "logger.h"

static void log_formatter(const char* tag, const char* message, va_list args) {   
	time_t now;     
	time(&now);     
	char* date = ctime(&now);   
	date[strlen(date) - 1] = '\0';  
	
	char* color;
	if (strcmp(tag, "INFO") == 0) {
		color = "\x1b[0;32m";
	} else if (strcmp(tag, "DEBUG") == 0) {
		color = "";
	} else if (strcmp(tag, "WARN") == 0) {
		color = "\x1b[0;33m";
	} else if (strcmp(tag, "ERROR") == 0) {
		color = "\x1b[0;31m";
	} else if (strcmp(tag, "CRITICAL") == 0) {
		color = "\x1b[0;91m";
	}
	
	printf("%s[%s] %s [%s.%s:%i] \x1b[0m", color, date, tag, __FILE__, __FUNCTION__, __LINE__);
	vprintf(message, args);     
}

void log_info(char* message, ...) {
	va_list args;
	va_start(args, message);
	log_formatter("INFO", message, args);
	va_end(args);
}

void log_debug(char* message, ...) {
	va_list args;
	va_start(args, message);
	log_formatter("DEBUG", message, args);
	va_end(args);
}

void log_warn(char* message, ...) {
	va_list args;
	va_start(args, message);
	log_formatter("WARN", message, args);
	va_end(args);
}

void log_error(char* message, ...) {
	va_list args;
	va_start(args, message);
	log_formatter("ERROR", message, args);
	va_end(args);
}

void log_critical(char* message, ...) {
	va_list args;
	va_start(args, message);
	log_formatter("CRITICAL", message, args);
	va_end(args);
}

