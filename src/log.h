/*======================================================================*
 * Copyright (c) 2008, Yahoo! Inc. All rights reserved.                 *
 * Copyright (c) 2010-2016, OpenX Inc.   All rights reserved.           *
 *                                                                      *
 * Licensed under the New BSD License (the "License"); you may not use  *
 * this file except in compliance with the License.  Unless required    *
 * by applicable law or agreed to in writing, software distributed      *
 * under the License is distributed on an "AS IS" BASIS, WITHOUT        *
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.     *
 * See the License for the specific language governing permissions and  *
 * limitations under the License. See accompanying LICENSE file.        *
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
