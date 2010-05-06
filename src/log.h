/*======================================================================*
 * Copyright (C) 2008 Light Weight Event System                         *
 * All rights reserved.                                                 *
 *                                                                      *
 * This program is free software; you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation; either version 2 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      *
 * This program is distributed in the hope that it will be useful,      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program; if not, write to the Free Software          *
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,                   *
 * Boston, MA 02110-1301 USA.                                           *
 *======================================================================*/
#ifndef LOG_H
#define LOG_H

#include "config.h"

#include <stdarg.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

typedef enum {
    LOG_OFF, LOG_ERROR, LOG_WARNING, LOG_INFO, LOG_PROGRESS
} log_level_t;

typedef enum {
  LOG_MASK_OFF = (1<<LOG_OFF),
  LOG_MASK_ERROR = (1<<LOG_ERROR),
  LOG_MASK_WARNING = (1<<LOG_WARNING),
  LOG_MASK_INFO = (1<<LOG_INFO),
  LOG_MASK_PROGRESS = (1<<LOG_PROGRESS)
} log_mask_t;

/*! \file log.h
 *  \brief Functions for logging within the journaller
 */

/*! \brief Logs a message
 *
 *  This method is not usually called directly, instead one of the
 *  logging macros is used.
 *
 *  \param[in] time the UTC time of the log line
 *  \param[in] level the level to log
 *  \param[in] fname the filename logging this message
 *  \param[in] lineno the line number of the file logging this message
 *  \param[in] format the format of the message
 */ 
void log_msg(log_level_t level, const char *fname, int lineno, const char* format, ...);
void log_get_mask_string(char* str, int len);

#if defined(HAVE_GCC_MACRO_VARARGS) || defined(HAVE_GNUC_C_VARARGS)
#define LOG_ER(args...)       log_msg(LOG_ERROR,    __FILE__, __LINE__, args)
#define LOG_WARN(args...)     log_msg(LOG_WARNING,  __FILE__, __LINE__, args)
#define LOG_INF(args...)      log_msg(LOG_INFO,     __FILE__, __LINE__, args)
#define LOG_PROG(args...)     log_msg(LOG_PROGRESS, __FILE__, __LINE__, args)
#elif defined(HAVE_ISO_MACRO_VARARGS) || defined(HAVE_ISO_C_VARARGS)
#define LOG_ER(...)           log_msg(LOG_ERROR,    __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)         log_msg(LOG_WARNING,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INF(...)          log_msg(LOG_INFO,     __FILE__, __LINE__, __VA_ARGS__)
#define LOG_PROG(...)         log_msg(LOG_PROGRESS, __FILE__, __LINE__, __VA_ARGS__)
#else
#error You must have some type (either ISO-C99 or GCC style) of variable argument list for macros.
#endif

#endif
