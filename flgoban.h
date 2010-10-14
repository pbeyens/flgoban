/*
Copyright (c) 2010, Pieter Beyens
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
Neither the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef FLGOBAN_H
#define FLGOBAN_H

#include <FL/Fl_Widget.H>
#include "goban.h"

/* marks */
enum {
	circle = 1,
};


class Fl_Goban: public Fl_Widget
{
	public:
		Fl_Goban(int x, int y, int w, int h, void (*cbk)(char));
		virtual void draw();
		virtual int set_stone(int x, int y, int val);
		virtual int set_mark(int x, int y, int val);
		virtual void flclear();
		virtual void flresize(int size);
		virtual int handle(int e);

	protected:
		virtual void draw_lines();
		virtual void draw_handi_dot(int x, int y);
		virtual void draw_stone(int x, int y, int color);
		virtual void draw_mark_circle(int x, int y);
		virtual void draw_dot(int x, int y, int color_edge, int color_fill, int handicap=0, int mark=0);

		inline int pos(int x, int y) { return x*size + y; }

		struct goban *g;
		int size;
		int *marks;
		void (*cb_key)(char key);

	private:
};

#endif
