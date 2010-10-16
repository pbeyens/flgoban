/*
Copyright (c) 2010, Pieter Beyens
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
Neither the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "sgf.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

#define SGF_PARSE_ERROR 0xffffffff
#define SGF_INVALID_SZ 0xff

static const struct sgf_cb *cb = 0;

static int is_color(const char* bp,const unsigned long pos)
{
	char c = bp[pos];
	if(c=='B' || c=='W') return 1;
	else return 0;
}

static int is_coord(const char* bp,const unsigned long pos)
{
	char c = bp[pos];
	if(c>='a' && c<'t') return 1;
	else return 0;
}

static unsigned long next_char(const char* bp,unsigned long pos,const unsigned long size)
{
	++pos;
	if(pos>=size)
		return SGF_PARSE_ERROR;
	char c = bp[pos];
	while(c==' ' || c=='\r' || c=='\n') {
		++pos;
		if(pos>=size)
			return SGF_PARSE_ERROR;
		c = bp[pos];
	}
	if(c=='\\') /* ignore escaped characters */
		return next_char(bp, pos+1, size);
	return pos;
}

static unsigned long read_propid(const char *id,const char* bp,unsigned long pos,const unsigned long size)
{
	int len = strlen(id);
	if(memcmp(id,bp+pos,len)==0 && bp[pos+len]=='[')
		return pos+len;
	else
		return pos;
}

static unsigned long read_propval_string(const char* bp,unsigned long pos,const unsigned long size)
{
	while(bp[pos]!=']') {
		pos = next_char(bp,pos,size);
		if(pos>=size)
			return SGF_PARSE_ERROR;
	}
	return pos;
}

static unsigned long read_unkown(const char* bp,unsigned long pos,const unsigned long size)
{
	unsigned long pos0 = pos;
	char c = bp[pos];

	while(c!=']')
	{
		pos=next_char(bp,pos,size);
		if(pos>=size) return SGF_PARSE_ERROR;
		c=bp[pos];
	}
	if(cb->prop_unknown)
		cb->prop_unknown(bp+pos0, pos-pos0+1);
	return pos; // ']'
}

static unsigned long read_play(const char* bp,unsigned long pos,const unsigned long size)
{
	unsigned long pos2 = pos;

	if(pos2>=size) return pos;
	if(!is_color(bp,pos2)) return pos;

	pos2 = next_char(bp,pos2,size);
	if(pos2>=size) return pos;
	if(bp[pos2]!='[') return pos;

	pos2 = next_char(bp,pos2,size);
	if(pos2>=size) return pos;
	if(is_coord(bp,pos2))
	{
		pos2 = next_char(bp,pos2,size);
		if(pos2>=size) return pos;
		if(!is_coord(bp,pos2)) return pos;

		pos2 = next_char(bp,pos2,size);
		if(pos2>=size) return pos;
		if(bp[pos2]!=']') return pos;
	
		if(bp[pos]=='B' && cb->b) cb->b(bp[pos+2],bp[pos+3]);
		else if(bp[pos]=='W' && cb->w) cb->w(bp[pos+2],bp[pos+3]);
		return pos2;
	}
	else if(bp[pos2]=='t') 
	{
		pos2 = next_char(bp,pos2,size);
		if(pos2>=size) return pos;
		if(bp[pos2]!='t') return pos;
		// pass move (SGF FF3)
		return pos2;
	}
	else if(bp[pos2]==']')
	{
		// pass move (SGF FF4)
		return pos2;
	}
	else return pos;
}

static unsigned long read_add(const char* bp,unsigned long pos,const unsigned long size)
{
	unsigned long pos2 = pos;
	unsigned char color;

	if(pos2>=size) return pos;
	if(bp[pos2]!='A') return pos;
	pos2 = next_char(bp,pos2,size);
	if(pos2>=size) return pos;
	if(!is_color(bp,pos2) && bp[pos2]!='E') return pos;
	color = bp[pos2];

	while(1) {
		pos2 = next_char(bp,pos2,size);
		if(pos2>=size) return pos;
		if(bp[pos2]!='[') return pos;

		pos2 = next_char(bp,pos2,size);
		if(pos2>=size) return pos;
		if(!is_coord(bp,pos2)) return pos;
		
		pos2 = next_char(bp,pos2,size);
		if(pos2>=size) return pos;
		if(!is_coord(bp,pos2)) return pos;

		pos2 = next_char(bp,pos2,size);
		if(pos2>=size) return pos;
		if(bp[pos2]!=']') return pos;

		if(color=='B' && cb->ab) cb->ab(bp[pos2-2],bp[pos2-1]);
		else if(color=='W' && cb->aw) cb->aw(bp[pos2-2],bp[pos2-1]);
		else if(color=='E' && cb->ae) cb->ae(bp[pos2-2],bp[pos2-1]);
	}
	return pos2;
}

static unsigned short read_sz(const char* bp,unsigned long pos,const unsigned long size)
{
	unsigned long pos2 = read_propid("SZ",bp,pos,size);
	if(pos2==pos)
		return pos;

	pos2 = next_char(bp,pos2,size);
	if(pos2>=size) return pos;
	if(bp[pos2]!='1') return pos;

	pos2 = next_char(bp,pos2,size);
	if(pos2>=size) return pos;
	if(bp[pos2]!='9') return pos;

	pos2 = next_char(bp,pos2,size);
	if(pos2>=size) return pos;
	if(bp[pos2]!=']') return pos;

	if(cb->sz)
		cb->sz(19);

	return pos2;
}

static unsigned long read_pw(const char* bp,unsigned long pos,const unsigned long size)
{
	unsigned long pos2 = read_propid("PW",bp,pos,size);
	if(pos2==pos)
		return pos;
	pos2 = read_propval_string(bp,pos2,size);
	if(cb->pw) cb->pw(bp+pos+3,pos2-pos-3);
	return pos2;
}

static unsigned long read_pb(const char* bp,unsigned long pos,const unsigned long size)
{
	unsigned long pos2 = read_propid("PB",bp,pos,size);
	if(pos2==pos)
		return pos;
	pos2 = read_propval_string(bp,pos2,size);
	if(cb->pb) cb->pb(bp+pos+3,pos2-pos-3);
	return pos2;
}

static unsigned short read_prop(const char* bp,unsigned long pos,const unsigned long size)
{
	unsigned long pos2 = read_sz(bp,pos,size);
	if(pos==pos2) pos2 = read_add(bp,pos,size);
	if(pos==pos2) pos2 = read_play(bp,pos,size);
	if(pos==pos2) pos2 = read_pw(bp,pos,size);
	if(pos==pos2) pos2 = read_pb(bp,pos,size);

	/* must be last -> just read the unknown property */
	if(pos==pos2) pos2 = read_unkown(bp,pos,size);
	return pos2;
}

static unsigned short read_node(const char* bp,unsigned long pos,const unsigned long size)
{
	if(bp[pos]!=';')
		return pos;
	pos = next_char(bp,pos,size);
	if(cb->node_new)
		cb->node_new();
	while(pos<size && bp[pos]!=';' && bp[pos]!=')' && bp[pos]!='(') {
		pos = read_prop(bp,pos,size);
		pos = next_char(bp,pos,size);
	}
	if(cb->node_end)
		cb->node_end();
	return --pos; /* do not include next ';' or ')' */
}

static int _sgf_fast_parse(const char* bp,unsigned long pos,const unsigned long size)
{
	while(pos<size)
	{
		if(bp[pos]==')')
		{
			// only the main var is parsed
			return 1;
		}
		else if(bp[pos]=='(')
		{
			int rv = _sgf_fast_parse(bp, ++pos, size);
			return rv;
		}
		pos = read_node(bp,pos,size);
		pos = next_char(bp,pos,size);
	}
	return 0;
}

int sgf_init(const struct sgf_cb *cbs)
{
	cb = cbs;
	return 0;
}

int sgf_parse_fast(const char *sgf)
{
	assert(cb);
	return _sgf_fast_parse(sgf, 0, strlen(sgf));
}


