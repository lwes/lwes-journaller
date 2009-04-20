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

#ifndef MARSHAL_DOT_H
#define MARSHAL_DOT_H

#include <netinet/in.h>

#define marshal_short(cp, x) { \
    uint16_t tmp = htons(x); \
    memcpy((uint16_t*)cp, &tmp, sizeof(tmp)); \
    cp = (unsigned char*)((uint16_t*)cp + 1); \
    }

#define marshal_long(cp, x) { \
    uint32_t tmp = htonl(x); \
    memcpy((uint16_t*)cp, &tmp, sizeof(tmp)); \
    cp = (unsigned char*)((uint32_t*)cp + 1); \
    }

#define marshal_long_long(cp, x) { \
    uint32_t tmp = htonl((unsigned long long)((x >> 32) & 0xFFFFFFFF)); \
    memcpy((uint32_t*)cp + 0, &tmp, sizeof(tmp)); \
    tmp = htonl((unsigned long long)(x & 0xFFFFFFFF)); \
    memcpy((uint32_t*)cp + 1, &tmp, sizeof(tmp)); \
    cp = (unsigned char*)((uint32_t*)cp + 2); \
    }

#define marshal_ulong_long(cp, x) { \
    uint32_t tmp = htonl((unsigned long long)((x >> 32) & 0xFFFFFFFF)); \
    memcpy((uint32_t*)cp + 0, &tmp, sizeof(tmp)); \
    tmp = htonl((unsigned long long)(x & 0xFFFFFFFF)); \
    memcpy((uint32_t*)cp + 1, &tmp, sizeof(tmp)); \
    cp = (unsigned char*)((uint32_t*)cp + 2); \
    }

#define unmarshal_short(cp, x) { \
    uint16_t tmp; \
    memcpy(&tmp, cp, sizeof(tmp)); \
    x = ntohs(tmp); \
    cp = (unsigned char*)((uint16_t*)cp + 1); \
    }

#define unmarshal_long(cp, x) { \
    uint32_t tmp; \
    memcpy(&tmp, cp, sizeof(tmp)); \
    x = ntohl(tmp); \
    cp = (unsigned char*)((uint32_t*)cp + 1); \
    }

#define unmarshal_long_long(cp, x) { \
    long long tmp; \
    memcpy(&tmp, cp, sizeof(tmp)); \
    x = ((unsigned long long)ntohl(*((uint32_t*)(&tmp) + 0)) << 32) \
                           + ntohl(*((uint32_t*)(&tmp) + 1)); \
    cp = (unsigned char*)((uint32_t*)cp + 2); \
    }

#define unmarshal_ulong_long(cp, x) { \
    uint32_t tmp[2]; \
    memcpy(&tmp, cp, sizeof(tmp)); \
    x = ((unsigned long long)ntohl(tmp[0]) << 32) \
                           + ntohl(tmp[1]); \
    cp = (unsigned char*)((uint32_t*)cp + 2); \
    }

#endif
