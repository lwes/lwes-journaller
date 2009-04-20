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

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#else
#define LOG_EMERG       0       /* system is unusable */
#define LOG_ALERT       1       /* action must be taken immediately */
#define LOG_CRIT        2       /* critical conditions */
#define LOG_ERR         3       /* error conditions */
#define LOG_WARNING     4       /* warning conditions */
#define LOG_NOTICE      5       /* normal but significant condition */
#define LOG_INFO        6       /* informational */
#define LOG_DEBUG       7       /* debug-level messages */
#endif

enum {
    off, error, warning, info, progress
};

#define LOG_MASK_OFF            (1<<off)
#define LOG_MASK_ERROR          (1<<error)
#define LOG_MASK_WARNING        (1<<warning)
#define LOG_MASK_INFO           (1<<info)
#define LOG_MASK_PROGRESS       (1<<progress)

/*! \file log.h
 *  \brief Functions for logging within the journaller
 */

/*! \brief Logs a message
 *
 *  This method is not usually called directly, instead one of the
 *  logging macros is used.
 *
 *  \param[in] level the syslog level to log
 *  \param[in] fname the filename logging this message
 *  \param[in] lineno the line number of the file logging this message
 *  \param[in] format the format of the message
 */ 
void log_msg(int level, char *fname, int lineno, const char* format, ...);
void log_get_level_string(char* str, int len);

#if defined(HAVE_GCC_MACRO_VARARGS) || defined(HAVE_GNUC_C_VARARGS)
#define LOG_ER(args...)       log_msg(LOG_MASK_ERROR,    __FILE__, __LINE__, args)
#define LOG_WARN(args...)     log_msg(LOG_MASK_WARNING,  __FILE__, __LINE__, args)
#define LOG_INF(args...)      log_msg(LOG_MASK_INFO,     __FILE__, __LINE__, args)
#define LOG_PROG(args...)     log_msg(LOG_MASK_PROGRESS, __FILE__, __LINE__, args)
#elif defined(HAVE_ISO_MACRO_VARARGS) || defined(HAVE_ISO_C_VARARGS)
#define LOG_ER(...)           log_msg(LOG_MASK_ERROR,    __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)         log_msg(LOG_MASK_WARNING,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INF(...)          log_msg(LOG_MASK_INFO,     __FILE__, __LINE__, __VA_ARGS__)
#define LOG_PROG(...)         log_msg(LOG_MASK_PROGRESS, __FILE__, __LINE__, __VA_ARGS__)
#else
#error You must have some type (either ISO-C99 or GCC style) of variable argument list for macros.
#endif

#endif
