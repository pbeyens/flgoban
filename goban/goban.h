/*
Copyright (c) 2010, Pieter Beyens
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
Neither the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef GOBAN_H
#define GOBAN_H

#ifdef __cplusplus
extern "C" {
#endif

struct goban;

enum {
	empty = 0,
	black,
	white,
};

struct goban_cb {
	void (*ab)(struct goban *, int, int);
	void (*aw)(struct goban *, int, int);
	void (*ae)(struct goban *, int, int);
};

extern int sgf2int(char c);
extern char int2sgf(int c);

extern struct goban *goban_alloc(int size, const struct goban_cb *gcb);
extern void goban_free(struct goban *g);
extern int goban_val(const struct goban *g, int x, int y);
extern int goban_is_black(const struct goban *g, int x, int y);
extern int goban_is_white(const struct goban *g, int x, int y);
extern int goban_is_empty(const struct goban *g, int x, int y);
extern int goban_libs(const struct goban *g, int x, int y);

extern void goban_set(struct goban *g, int x, int y, int val);
extern void goban_play(struct goban *g, int x, int y, int val);

#ifdef __cplusplus
}
#endif

#endif
