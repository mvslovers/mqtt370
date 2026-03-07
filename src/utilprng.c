#include "mqttutil.h"

/* 
*** Borrowed from https://prng.di.unimi.it/xoshiro128plusplus.c ***

Written in 2019 by David Blackman and Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide.

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/*** Modified by Mike Rayborn for MVS38J with CRENT370. Use at your own risk ***/

/* wsakey[] is used as a key and model for writable static that will
 * be allocated by the __wsaget() function.
 */
static uint32_t wsakey[5] = {0,0,0,0,0};

static inline uint32_t rotl(const uint32_t x, int k) 
{
	return (x << k) | (x >> (32 - k));
}

static uint32_t *prng_state(void) 
{
    uint32_t *s = __wsaget(wsakey, sizeof(wsakey));

    lock(s, LOCK_EXC);

    if (s[4]==0) {
        uclock64_t *u1 = (uclock64_t*)&s[0];
        
        // wtof("%s: Initializing", __func__);

        /* initialize prng state (seed the state) */
        *u1 = uclock64();
        s[2] = rotl(s[1], 11);
        s[0] = rotl(s[0]+s[2], 5) + s[1];
        s[3] = rotl(s[0]+s[2], 7) + s[1];
        
        s[4] = 1; /* state is initialized flag */
        // wtodumpf(s, 16, "%s: prng_state", __func__);
    }
    
    return s;
}

uint32_t util_prng(void) 
{
    uint32_t        *s      = prng_state();
	const uint32_t  result  = rotl(s[0] + s[3], 7) + s[0];
	const uint32_t  t       = s[1] << 9;

	s[2] ^= s[0];
	s[3] ^= s[1];
	s[1] ^= s[2];
	s[0] ^= s[3];

	s[2] ^= t;

	s[3] = rotl(s[3], 11);

    unlock(s, LOCK_EXC);

	return result;
}
