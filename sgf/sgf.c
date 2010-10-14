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
#include <assert.h>

#define SGF_PARSE_ERROR 0xffffffff
#define SGF_INVALID_SZ 0xff

static const struct sgf_cb *cb = 0;

static unsigned long next_char(const char* bp,unsigned long pos,const unsigned long size)
{
	if(pos<size) ++pos;
	char c = bp[pos];
	while( pos<size && (c==' ' || c=='\r' || c=='\n')) { ++pos; c = bp[pos]; }
	if(pos>=size) return SGF_PARSE_ERROR;
	if(cb->node && bp[pos]==';' && bp[pos-1]!='\\')
		cb->node();
	return pos;
}

static unsigned long fp_next_prop(const char* bp,unsigned long pos,const unsigned long size)
{
	// in fast parsing, a ')' means the main var is done
	int inProp = 1;
	char c = bp[pos];
	if(c==')') {
		if(cb->node) cb->node();
		return pos;
	}
	else if(c=='(') return pos;
	while(pos<size && c!='[')
	{
		pos=next_char(bp,pos,size);
		c=bp[pos];
		if(c==']') inProp = 0;
		else if(c==')' && !inProp) break;
	}
	if(pos>=size) return SGF_PARSE_ERROR;
	if(c==')') return pos;

	// go back to first upper capital of property ident
	assert(c=='[');
	--pos;
	c=bp[pos];
	while(c>='A' && c<='Z') { --pos; c=bp[pos]; }
	++pos;
	return pos;
}

static unsigned long fp_skip_prop(const char* bp,unsigned long pos,const unsigned long size)
{
	char c = bp[pos];
	while(pos<size && c!='[' && c!=')')
	{
		pos=next_char(bp,pos,size);
		c=bp[pos];
	}
	if(pos>=size) return SGF_PARSE_ERROR;
	if(c==')') return size;
	return ++pos; // character after '['
}

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

static unsigned long read_play(const char* bp,unsigned long pos,const unsigned long size)
{
	unsigned long pos2 = pos;

	//cerr << bp[pos2] << " ";
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
	
		//cerr << bp[pos]  << bp[pos+2] << bp[pos+3] << endl;
		//unsigned short x = (unsigned short)bp[pos+2]-'a';
		//unsigned short y = (unsigned short)bp[pos+3]-'a';
		//cerr << "Y=" << y << endl;
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
		//cerr << bp[pos] << "pass" << endl;
		//p_gtree->pass((bp[pos]=='B') ? COLOR1:COLOR2);
		return pos2;
	}
	else if(bp[pos2]==']')
	{
		// pass move (SGF FF4)
		//cerr << bp[pos] << "pass" << endl;
		//p_gtree->pass((bp[pos]=='B') ? COLOR1:COLOR2);
		return pos2;
	}
	else return pos;
}

static unsigned long read_add(const char* bp,unsigned long pos,const unsigned long size)
{
	unsigned long pos2 = pos;
	unsigned char color;

	//cerr << bp[pos2] << " ";
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

		//cerr << color << bp[pos2-2] << bp[pos2-1] << endl;
		//unsigned short x = (unsigned short)bp[pos2-2]-'a';
		//unsigned short y = (unsigned short)bp[pos2-1]-'a';
		//cerr << "X=" << x << endl;
		//cerr << "Y=" << y << endl;
		if(color=='B' && cb->ab) cb->ab(bp[pos2-2],bp[pos2-1]);
		else if(color=='W' && cb->aw) cb->aw(bp[pos2-2],bp[pos2-1]);
		else if(color=='E' && cb->ae) cb->ae(bp[pos2-2],bp[pos2-1]);
	}
	return pos2;
}

static unsigned short read_sz(const char* bp,unsigned long pos,const unsigned long size)
{
	unsigned long pos2 = pos;
	if(pos2>=size) return pos;
	if(bp[pos2]!='S') return pos;

	pos2 = next_char(bp,pos2,size);
	if(pos2>=size) return pos;
	if(bp[pos2]!='Z') return pos;

	pos2 = next_char(bp,pos2,size);
	if(pos2>=size) return pos;
	if(bp[pos2]!='[') return pos;
	
	// from here on we have the correct property

	pos2 = next_char(bp,pos2,size);
	if(pos2>=size) return pos;
	if(bp[pos2]!='1') return pos;

	pos2 = next_char(bp,pos2,size);
	if(pos2>=size) return pos;
	if(bp[pos2]!='9') return pos;

	pos2 = next_char(bp,pos2,size);
	if(pos2>=size) return pos;
	if(bp[pos2]!=']') return pos;

	cb->sz(19);

	return pos2;
}

static int _sgf_fast_parse(const char* bp,unsigned long pos,const unsigned long size)
{
	while(pos<size)
	{
		pos = fp_next_prop(bp,pos,size);

		if(pos==SGF_PARSE_ERROR) break;
		if(bp[pos]==')')
		{
			//cerr << filename << " successfully parsed" << endl;
			return 1;
		}
		else if(bp[pos]=='(')
		{
			int rv = _sgf_fast_parse(bp, ++pos, size);
			//gtree->up();
			return rv;
		}

		unsigned long pos2 = read_sz(bp,pos,size);
		if(pos==pos2) pos2 = read_add(bp,pos,size);
		if(pos==pos2) pos2 = read_play(bp,pos,size);
		if(pos==pos2) pos2 = fp_skip_prop(bp,pos,size);
		pos = next_char(bp,pos2,size);
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


