/*
Copyright (c) 2010, Pieter Beyens
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
Neither the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "goban.h"

#include <stdlib.h>
#include <string.h>

#include <set>
using namespace std;

struct coord { int x; int y; };
static bool operator<(const coord& c1, const coord& c2)
{
	if(c1.x*19+c1.y < c2.x*19+c2.y)
		return 1;
	return 0;
}

struct goban {
	int size;
	int *vals;
	const struct goban_cb *cb;
};

static int pos(const struct goban *g, int x, int y)
{
	return x*g->size + y;
}

static void get_info(const struct goban *g,int color,int x,int y,set<struct coord>& mystones,set<struct coord>& libs, set<struct coord>& all)
{
	struct coord c = {x,y};
	if(all.find(c) != all.end())
		return;
	all.insert(c);
	int v = goban_val(g,x,y);
	if(v==empty) {
		libs.insert(c);
		return;
	}
	else if(v==color) {
		mystones.insert(c);
		if(x > 0)
			get_info(g, color, x-1, y, mystones, libs, all);
		if(x < g->size-1)
			get_info(g, color, x+1, y, mystones, libs, all);
		if(y > 0)
			get_info(g, color, x, y-1, mystones, libs, all);
		if(y < g->size-1)
			get_info(g, color, x, y+1, mystones, libs, all);
	}
}

struct goban *goban_alloc(int size, const struct goban_cb *gcb)
{
	struct goban *g = (struct goban*) malloc(sizeof(struct goban));
	g->size = size;
	g->vals = (int*)calloc(size*size,sizeof(int));
	g->cb = gcb;
	return g;
}

void goban_free(struct goban *g)
{
	if(g->vals) {
		for(int i=0;i<(g->size*g->size);++i) {
			free((int*)&g->vals[i]);
		}
	}
	if(g)
		free(g);
}

int goban_val(const struct goban *g, int x, int y)
{
	return g->vals[pos(g,x,y)];
}

int goban_is_black(const struct goban *g, int x, int y)
{
	return black == g->vals[pos(g,x,y)];
}

int goban_is_white(const struct goban *g, int x, int y)
{
	return white == g->vals[pos(g,x,y)];
}

int goban_is_empty(const struct goban *g, int x, int y)
{
	return empty == g->vals[pos(g,x,y)];
}

int goban_libs(const struct goban *g, int x, int y)
{
	set<struct coord> stones, libs, all;
	get_info(g, goban_val(g,x,y), x, y, stones, libs, all);
	return libs.size();
}

void goban_set(struct goban *g, int x, int y, int val)
{
	g->vals[pos(g,x,y)] = val;
	if(!g->cb)
		return;
	if(g->cb->ae && val == empty)
		g->cb->ae(g,x,y);
	else if(g->cb->ab && val == black)
		g->cb->ab(g,x,y);
	else if(g->cb->aw && val == white)
		g->cb->aw(g,x,y);
}

static void goban_remove_if_no_libs(struct goban *g, int x, int y)
{
	set<struct coord> mystones, all, libs;
	set<struct coord>::const_iterator it;
	get_info(g, goban_val(g,x,y), x, y, mystones, libs, all);
	if(libs.size())
		return;
	for(it=mystones.begin();it!=mystones.end();++it) {
		goban_set(g, it->x, it->y, empty);
	}
}

void goban_play(struct goban *g, int x, int y, int val)
{
	goban_set(g,x,y,val);
	if(x>0 && goban_val(g,x-1,y)!=val) goban_remove_if_no_libs(g,x-1,y);
	if(x<g->size-1 && goban_val(g,x+1,y)!=val) goban_remove_if_no_libs(g,x+1,y);
	if(y>0 && goban_val(g,x,y-1)!=val) goban_remove_if_no_libs(g,x,y-1);
	if(y<g->size-1 && goban_val(g,x,y+1)!=val) goban_remove_if_no_libs(g,x,y+1);
}

int sgf2int(char c)
{
	return c - 'a';
}

char int2sgf(int c)
{
	return c + 'a';
}

int goban_size(const struct goban *g)
{
	return g->size;
}

