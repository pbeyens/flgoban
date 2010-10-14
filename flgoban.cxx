/*
Copyright (c) 2010, Pieter Beyens
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
Neither the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <FL/fl_draw.H>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <FL/Fl.H>

#include "flgoban.h"

struct coord {
	int x;
	int y;
};

const static struct coord handi19[] = {
	{3,3}, {3,9}, {3,15},
	{9,3}, {9,9}, {9,15},
	{15,3}, {15,9}, {15,15}
};

const static struct coord handi13[] = {
	{3,3}, {3,9}, {9,9}, {9,3},
	{6,6}
};

const static struct coord handi9[] = {
	{2,2}, {6,2}, {2,6}, {6,6}
};


Fl_Goban::Fl_Goban(int x, int y, int w, int h, void (*cbk)(char)): Fl_Widget(x,y,w,h)
{
	cb_key = cbk;
	g = NULL;
	marks = NULL;
	flresize(19);
}

void Fl_Goban::draw()
{
	/* draw background */
	//fl_draw_box(FL_FLAT_BOX, 0, 0, w(), h(), fl_rgb_color(0x6E,0x93,0xA5));
	fl_draw_box(FL_FLAT_BOX, 0, 0, w(), h(), fl_rgb_color(90,220,180));
	//fl_draw_box(FL_FLAT_BOX, 0, 0, w(), h(), fl_rgb_color(0xCC,0xCC,0xCC));

	/* borders and lines */
	draw_lines();

	/* handicap dots */
	if(size == 19) {
		for(int i=0;i<9;++i)
			draw_handi_dot(handi19[i].x,handi19[i].y);
	} else if(size == 13) {
		for(int i=0;i<5;++i)
			draw_handi_dot(handi13[i].x,handi13[i].y);
	} else if(size == 9) {
		for(int i=0;i<4;++i)
			draw_handi_dot(handi9[i].x,handi9[i].y);
	}

	/* stones and marks */
	for(int x=0;x<size;++x)
		for(int y=0;y<size;++y) {
			if(!goban_is_empty(g,x,y))
				draw_stone(x,y,goban_val(g,x,y));
			if(marks[pos(x,y)]==circle) {
				draw_mark_circle(x,y);
			}
	}

}

void Fl_Goban::draw_lines()
{
	int offset_x = 0;
	int offset_y = 0;
	int min;
	if(w() > h()) {
		offset_x = (w() - h()) / 2;
		min = h();
	}
	else if(h() > w()) {
		offset_y = (h() - w()) / 2;
		min = w();
	}
	else min = w();

	
	int i;
	float d = min/(size+1);
	fl_rectf(d+offset_x-5, d+offset_y-5, 12+((size-1)*d), 12+((size-1)*d), fl_rgb_color(220,180,90));
	fl_color(FL_BLACK);
	for(i=0;i<size;++i) {
		int x0, x1, y0, y1;
		x0 = x1 = d + i*d;
		y0 = d;
		y1 = d + (size-1)*d;
		if(i==0 || i==size-1)
			fl_line_style(FL_SOLID, 2, 0);
		else
			fl_line_style(FL_SOLID, 1, 0);
		fl_line(x0+offset_x,y0+offset_y,x1+offset_x,y1+offset_y); /* vertical */
		fl_line(y0+offset_x,x0+offset_y,y1+offset_x,x1+offset_y); /* horizontal */
	}
}

void Fl_Goban::draw_dot(int x, int y, int color_edge, int color_fill, int handicap, int mark)
{
	int offset_x = 0;
	int offset_y = 0;
	int min;
	if(w() > h()) {
		offset_x = (w() - h()) / 2;
		min = h();
	}
	else if(h() > w()) {
		offset_y = (h() - w()) / 2;
		min = w();
	}
	else min = w();
	float d = min/(size+1);
	int c_x = d + x*d;
	int c_y = d + y*d;
	int dotsize = 0;

	if(handicap)
		dotsize = d/4;
	else if(mark)
		dotsize = d/2;
	else
		dotsize = d-4;

	if(color_fill) {
		fl_color(color_fill);
		fl_pie(c_x+offset_x-(dotsize/2),c_y+offset_y-(dotsize/2), dotsize, dotsize, 0, 360);
	}

	fl_color(color_edge);
	if(mark)
		fl_line_style(FL_SOLID, 3, 0);
	else
		fl_line_style(FL_SOLID, 1, 0);
	fl_arc(c_x+offset_x-(dotsize/2),c_y+offset_y-(dotsize/2), dotsize, dotsize, 0, 360);

}

void Fl_Goban::draw_mark_circle(int x, int y)
{
	if(goban_is_black(g,x,y))
		draw_dot(x,y,FL_WHITE,0,0,1);
	else
		draw_dot(x,y,FL_BLACK,0,0,1);
}

int Fl_Goban::set_stone(int x, int y, int val)
{
	goban_set(g, x, y, val);
	return 0;
}

int Fl_Goban::set_mark(int x, int y, int val)
{
	if(x >= size || y >= size) {
		printf("coord out of range");
		return -1;
	}
	marks[pos(x,y)] = val;
	return 0;
}

void Fl_Goban::flclear()
{
	flresize(size);
}

void Fl_Goban::flresize(int s)
{
	size = s;
	g = goban_alloc(s, NULL);

	if(marks) free(marks);
	marks = (int*)calloc(size*size,sizeof(int));
}

void Fl_Goban::draw_handi_dot(int x, int y)
{
	draw_dot(x,y,FL_BLACK,FL_BLACK,1);
}

void Fl_Goban::draw_stone(int x, int y, int color)
{
	if(color == black)
		draw_dot(x,y,FL_BLACK, FL_BLACK);
	else
		draw_dot(x,y,FL_BLACK, FL_WHITE);
}

int Fl_Goban::handle(int e)
{
	if(e == FL_KEYDOWN)
		cb_key(Fl::event_key());
}

