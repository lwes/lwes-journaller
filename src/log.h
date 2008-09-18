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
void log_msg(int level, const char *fname, const int lineno, 
             const char* format, ...);

#if defined(HAVE_GNUC_C_VARARGS)
#define LOG_MSG_EMERG(args,...)   log_msg(LOG_EMERG, __FILE__, __LINE__, args)
#define LOG_MSG_ALERT(args,...)   log_msg(LOG_ALERT, __FILE__,__LINE__, args)
#define LOG_MSG_CRIT(args,...)    log_msg(LOG_CRIT, __FILE__, __LINE__, args)
#define LOG_MSG_ERR(args,...)     log_msg(LOG_ERR, __FILE__, __LINE__, args)
#define LOG_MSG_WARNING(args,...) log_msg(LOG_WARNING, __FILE__, __LINE__, args)
#define LOG_MSG_NOTICE(args,...)  log_msg(LOG_NOTICE, __FILE__, __LINE__, args)
#define LOG_MSG_INFO(args,...)    log_msg(LOG_INFO, __FILE__, __LINE__, args)
#define LOG_MSG_DEBUG(args,...)   log_msg(LOG_DEBUG, __FILE__, __LINE__, args)
#elif defined(HAVE_ISO_C_VARARGS)
#define LOG_MSG_EMERG(...)   log_msg(LOG_EMERG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_MSG_ALERT(...)   log_msg(LOG_ALERT, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_MSG_CRIT(...)    log_msg(LOG_CRIT, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_MSG_ERR(...)     log_msg(LOG_ERR, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_MSG_WARNING(...) log_msg(LOG_WARNING, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_MSG_NOTICE(...)  log_msg(LOG_NOTICE, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_MSG_INFO(...)    log_msg(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_MSG_DEBUG(...)   log_msg(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)

#else
#error You must have some type (either ISO-C99 or GCC style) of variable argument list for macros.
#endif

#endif

