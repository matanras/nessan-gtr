#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <log.h>

struct logger {
	char *logger_name;
	FILE *output_file;
};

#define LOG_FILE "log.txt"
#define NEWLINE "\n"
#define EMPTY_SPACE " "

static char *log_lvl_banners[] = {
	[INFO] = "[INFO]",
	[WARNING] = "[WARNING]",
	[ERROR] = "[ERROR]",
	[FATAL] = "[FATAL]",
	[DEBUG] = "[DEBUG]"
};

static void log_current_time(FILE *log_file)
{
	time_t current_time;
	struct tm *time_info;
	char time_string[9];  /* space for "HH:MM:SS\0" */

	time(&current_time);
	time_info = localtime(&current_time);

	strftime(time_string, sizeof(time_string), "%H:%M:%S", time_info);
	
	/* Sizeof - 1 to avoid printing null byte to file. */
	fwrite(time_string, 1, sizeof(time_string) - 1, log_file);
}

int log_init(char *logger_name, char *output_fname, logger_t **out_logger)
{
	FILE *output_file;
	struct logger *logger = malloc(sizeof(struct logger));

	if (!logger)
		return -1;

	output_file = fopen(output_fname, "a");

	if (!output_file)
		return -1;

	logger->logger_name = logger_name;
	logger->output_file = output_file;
	*out_logger = logger;

	return 0;
}

void log_destroy(logger_t *logger)
{
	fclose(logger->output_file);
	free(logger);
}

static void log_logger_name(const struct logger *logger)
{
	fwrite("[", 1, 1, logger->output_file);
	fwrite(logger->logger_name, 1, strlen(logger->logger_name), logger->output_file);
	fwrite("]", 1, 1, logger->output_file);
}

void log(logger_t *logger, const enum log_level log_lvl, char *fmt, ...)
{
	va_list args;

	log_current_time(logger->output_file);
	fwrite(EMPTY_SPACE, 1, 1, logger->output_file);

	log_logger_name(logger);
	fwrite(EMPTY_SPACE, 1, 1, logger->output_file);

	fwrite(log_lvl_banners[log_lvl], 1, strlen(log_lvl_banners[log_lvl]), logger->output_file);
	fwrite(EMPTY_SPACE, 1, 1, logger->output_file);

	va_start(args, fmt);
	vfprintf(logger->output_file, fmt, args);
	va_end(args);

	fwrite(NEWLINE, 1, 1, logger->output_file);
}
