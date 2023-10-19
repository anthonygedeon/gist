#ifndef LOGGER_H
#define LOGGER_H

void log_info(char* message, ...);
void log_debug(char* message, ...);
void log_warn(char* message, ...);
void log_error(char* message, ...);
void log_critical(char* message, ...);

#endif
