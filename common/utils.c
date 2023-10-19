#include <string.h>
#include <ctype.h>

#include "utils.h"

char* to_lowercase(const char* s) {
	static char buffer[1024];
	for (int i = 0; s[i] != '\0' ; i++) {
		buffer[i] = tolower(s[i]);
	}
    buffer[strlen(buffer)] = '\0';
	return buffer;
}
