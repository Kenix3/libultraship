#pragma once

/*
 * Below are the various copyright notices from the original source.
 * <http://www.mirrorservice.org/sites/download.sourceforge.net/pub/
 *	sourceforge/l/li/libcore/libcore_0.22.7.tar.gz/libcore/lib/hash/Crc.cpp>
 *
 * The source has be substationally modified:
 *	1. Unneeded code has been deleted.
 *	2. The code was converted from C++ to C.
 *	3. Routines optimized for hashing were added.
 */
///////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2006 Anton Samokhvalov
//
//  Permission is hereby  granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without  restriction, including without limitation the rights
//  to use, copy, modify,  merge,  publish,  distribute,  sublicense, and/or sell
//  copies  of  the  Software,  and  to  permit  persons  to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The  above copyright notice and  this permission  notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED  "AS IS",  WITHOUT WARRANTY  OF ANY KIND, EXPRESS OR
//  IMPLIED,  INCLUDING BUT  NOT LIMITED TO  THE WARRANTIES  OF  MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR  PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
//  AUTHORS  OR  COPYRIGHT  HOLDERS  BE  LIABLE  FOR  ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN  CONNECTION  WITH THE  SOFTWARE OR THE USE OR  OTHER DEALINGS IN
//  THE SOFTWARE.
//
///////////////////////////////////////////////////////////////////////////////////
/*
	[To some parts of this code]
*/
/*
  Copyright (C) 1995-2004 Jean-loup Gailly and Mark Adler
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.
  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:
  1. The origin of this software must not be misrepresented; you must not
         claim that you wrote the original software. If you use this software
         in a product, an acknowledgment in the product documentation would be
         appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
         misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
  Jean-loup Gailly        Mark Adler
  jloup@gzip.org          madler@alumni.caltech.edu
*/
/*
	[To some parts of this code]
*/
/*
 Copyright (c) 2003, Dominik Reichl <dominik.reichl@t-online.de>
 All rights reserved.
 LICENSE TERMS
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
 * Neither the name of ReichlSoft nor the names of its contributors may be used
       to endorse or promote products derived from this software without specific
       prior written permission.
 DISCLAIMER
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <stdint.h>

/** @brief Initial CRC64 seed value. Pass to update_crc64() as the starting @p crc. */
#define INITIAL_CRC64 0xffffffffffffffffULL

/**
 * @brief Updates a running CRC64 checksum with @p len bytes from @p buf.
 *
 * Use this function to compute the CRC64 of a multi-part message by chaining calls:
 * @code
 * uint64_t crc = INITIAL_CRC64;
 * crc = update_crc64(part1, len1, crc);
 * crc = update_crc64(part2, len2, crc);
 * @endcode
 *
 * @param buf Pointer to the data to hash.
 * @param len Number of bytes in @p buf.
 * @param crc Running CRC64 value (initialise with INITIAL_CRC64).
 * @return Updated CRC64 value.
 */
extern uint64_t update_crc64(const void* buf, uint32_t len, uint64_t crc);

/**
 * @brief Computes the CRC64 of @p len bytes starting at @p buf.
 *
 * Equivalent to calling update_crc64() once with INITIAL_CRC64 as the seed.
 *
 * @param buf Pointer to the data to hash.
 * @param len Number of bytes in @p buf.
 * @return 64-bit CRC hash of the input.
 */
extern uint64_t crc64(const void* buf, uint32_t len);

/**
 * @brief Computes the CRC64 of a null-terminated string @p t.
 *
 * This is the primary hashing function used by the resource system to map virtual
 * resource paths (char*) to their 64-bit CRC identifiers.
 *
 * @param t Null-terminated string to hash.
 * @return 64-bit CRC hash of @p t (excluding the null terminator).
 */
extern uint64_t CRC64(const char* t);