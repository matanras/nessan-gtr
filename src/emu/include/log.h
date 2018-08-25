#pragma once

struct logger;
typedef const struct logger logger_t;

enum log_level {
	INFO,
	WARNING,
	ERROR,
	FATAL,
	DEBUG
};

/**
 * Init a logger.
 * @param logger_name The name of the logger.
 * @param output_fname The name of the output file. All log operations on the returned logger will
 *                     be written to the given file.
 * @param out_logger (OUT) A pointer to a logger handle.
 * @return 0 if no errors, -1 if the logger could not be created.
 */
int log_init(char *logger_name, char *output_fname, logger_t **out_logger);

/**
 * Destroy a logger.
 */
void log_destroy(logger_t *logger);

/**
 * Print info to the log file (log.txt).
 * @param logger The handle to the logger.
 * @param log_lvl The log level.
 * @param fmt The format (printf style).
 */
void log(logger_t *logger, const enum log_level log_lvl, char *fmt, ...);