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
