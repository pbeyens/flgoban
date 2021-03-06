/*
Copyright (c) 2010, Pieter Beyens
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
Neither the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Button.H>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <list>
#include <signal.h>
#include <argp.h>

#include "flgoban.h"
#include "sgf.h"
#include "goban.h"

using namespace std;

static Fl_Window *win = NULL;
static Fl_Goban *flgoban = NULL;
//static Fl_Text_Buffer *textbuffer = NULL;
//static Fl_Text_Editor *comment = NULL;
static list<int> fds;
static struct goban *g = NULL;
static char broadcast_msg[10000];
static int rotation;

struct settings
{
	int width, height, port, nogui, expand, nomark, swap, rotate;
};
static struct settings setts;

/* GOBAN */
static void rotate(int size, int x, int y, int * new_x, int * new_y)
{
	switch(rotation) {
		case 0:
			*new_x = x;
			*new_y = y;
			break;
		case 1:
			*new_x = size-1-x;
			*new_y = y;
			break;
		case 2:
			*new_x = x;
			*new_y = size-1-y;
			break;
		case 3:
			*new_x = size-1-x;
			*new_y = size-1-y;
			break;
		case 4:
			*new_x = y;
			*new_y = x;
			break;
		case 5:
			*new_x = size-1-y;
			*new_y = x;
			break;
		case 6:
			*new_x = y;
			*new_y = size-1-x;
			break;
		case 7:
			*new_x = size-1-y;
			*new_y = size-1-x;
			break;
	};
}

static void goban_ab(struct goban *gob, int x, int y)
{
	char msg[10];
	flgoban->set_stone(x,y,black);
	if(setts.expand) {
		sprintf(msg, "AB[%c%c]", int2sgf(x),int2sgf(y));
		strcat(broadcast_msg, msg);
	}
}

static void goban_aw(struct goban *gob, int x, int y)
{
	char msg[10];
	flgoban->set_stone(x,y,white);
	if(setts.expand) {
		sprintf(msg, "AW[%c%c]", int2sgf(x),int2sgf(y));
		strcat(broadcast_msg, msg);
	}
}

static void goban_ae(struct goban *gob, int x, int y)
{
	char msg[10];
	flgoban->set_stone(x,y,empty);
	if(setts.expand) {
		sprintf(msg, "AE[%c%c]", int2sgf(x),int2sgf(y));
		strcat(broadcast_msg, msg);
	}
}

static const struct goban_cb gcb = { goban_ab, goban_aw, goban_ae };

static struct goban * new_goban(int size)
{
	struct goban * gob = NULL;
	gob = goban_alloc(size, &gcb);
	return gob;
}

/* SGF */
static void sgf_node_new(void)
{
	flgoban->clear_marks();
	strcat(broadcast_msg, ";");
}

static void sgf_sz(int s)
{
	char msg[10];
	goban_free(g);
	g = new_goban(s);
	if(setts.rotate)
		rotation = rand() % 8;
	else
		rotation = 0;
	flgoban->flresize(s);
	sprintf(msg, "SZ[%d]", s);
	strcat(broadcast_msg, msg);
}

static void sgf_move(char col, char cx, char cy)
{
	char msg[20];
	int new_x, new_y;

	if(setts.swap) {
		if(col=='B') col='W';
		else col='B';
	}

	rotate(goban_size(g),sgf2int(cx),sgf2int(cy),&new_x,&new_y);

	goban_play(g, new_x, new_y, col=='B' ? black : white);
	if(!setts.expand) {
		sprintf(msg, "%c[%c%c]", col,int2sgf(new_x),int2sgf(new_y));
		strcat(broadcast_msg, msg);
	}

	if(!setts.nomark) {
		flgoban->set_mark(new_x,new_y,circle);
		if(setts.expand) {
			sprintf(msg, "CR[%c%c]", int2sgf(new_x),int2sgf(new_y));
			strcat(broadcast_msg, msg);
		}
	}
}

static void sgf_b(char cx, char cy)
{
	sgf_move('B',cx,cy);
}

static void sgf_w(char cx, char cy)
{
	sgf_move('W',cx,cy);
}

static void sgf_add(char col, char cx, char cy)
{
	char msg[10];
	int new_x, new_y;

	if(setts.swap) {
		if(col=='B') col='W';
		else col='B';
	}

	rotate(goban_size(g),sgf2int(cx),sgf2int(cy),&new_x,&new_y);

	goban_set(g, new_x, new_y, col=='B' ? black:white);
	if(!setts.expand) {
		sprintf(msg, "A%c[%c%c]",col,int2sgf(new_x),int2sgf(new_y));
		strcat(broadcast_msg, msg);
	}
}

static void sgf_ab(char cx, char cy)
{
	sgf_add('B',cx,cy);
}

static void sgf_aw(char cx, char cy)
{
	sgf_add('W',cx,cy);
}

static void sgf_ae(char cx, char cy)
{
	char msg[10];
	int new_x, new_y;

	rotate(goban_size(g),sgf2int(cx),sgf2int(cy),&new_x,&new_y);
	goban_set(g, new_x, new_y, empty);
	if(!setts.expand) {
		sprintf(msg, "AE[%c%c]",int2sgf(new_x) ,int2sgf(new_y));
		strcat(broadcast_msg, msg);
	}
}

static void sgf_pw(const char *prop, int size)
{
	strcat(broadcast_msg, "PW[");
	strncat(broadcast_msg, prop, size);
	strcat(broadcast_msg, "]");
}

static void sgf_pb(const char *prop, int size)
{
	strcat(broadcast_msg, "PB[");
	strncat(broadcast_msg, prop, size);
	strcat(broadcast_msg, "]");
}

static void sgf_cr(char cx, char cy)
{
	char msg[10];
	int new_x, new_y;

	rotate(goban_size(g),sgf2int(cx),sgf2int(cy),&new_x,&new_y);

	flgoban->set_mark(new_x,new_y,circle);
	sprintf(msg, "CR[%c%c]", int2sgf(cx),int2sgf(cy));
	strcat(broadcast_msg, msg);
}

static void sgf_prop_unknown(const char *prop, int size)
{
	strncat(broadcast_msg, prop, size);
}

static const struct sgf_cb cbs = {
	sgf_node_new,
	sgf_sz,
	sgf_b, sgf_w, sgf_ab, sgf_aw, sgf_ae,
	sgf_pw, sgf_pb,
	sgf_cr,
	NULL,
	sgf_prop_unknown
};

static void handler_sigpipe(int)
{
	return;
}

static void broadcast(const char *cmd)
{
	list<int>::const_iterator cit = fds.begin();
	while(cit!=fds.end()) {
		send(*cit, cmd, strlen(cmd),0);
		++cit;
	}
	printf("%s\n",cmd);
}

static char rd_cmd[10000];
static char *wr, *rd;
static void read_cb(int fd, void *data)
{
	int n;
	memset(broadcast_msg, '\0', sizeof(rd_cmd));
	n = read(fd, wr, 2048);
	if(n <= 0) {
		Fl::remove_fd(fd);
		fds.remove(fd);
		/* if not a gui and not listening then exit */
		if(setts.nogui && !setts.port && !fds.size())
			exit(0);
		return;
	}
	wr += n;
	rd = (char*) sgf_parse_fast(&cbs,rd);
	if(rd == wr) {
		memset(rd_cmd,'\0',sizeof(rd_cmd));
		rd = wr = rd_cmd;
	}
	broadcast(broadcast_msg);
	flgoban->redraw();
}

static void listen_cb(int fd, void *s)
{
	int a = accept(*(int*)s, NULL, 0);
	if(0 > a)
		perror("accept()");
	else {
		Fl::add_fd(a, read_cb, NULL);
		fds.push_back(a);
	}
}

/* EVENTS */

static void ev_key(char k)
{
	char e[100];
	sprintf(e, ";KEY[%c]\n", k);
	broadcast(e);
	if(k == 'q')
		exit(0);
}

static void ev_mou(int x, int y)
{
	char e[100];
	sprintf(e, ";MOU[%c%c]\n", int2sgf(x),int2sgf(y));
	broadcast(e);
}


/* ARGP */
const char *argp_program_version = "flgoban 0.1";
const char *argp_program_bug_address = "<pieter.beyens@gmail.com>";
static char doc[] = "flgoban -- reads, displays and broadcasts sgf";
static char args_doc[] = "";
static struct argp_option options[] = {
	{"width", 'w', "WIDTH", 0, "window width (defaults to 600)" },
	{"height", 'h', "HEIGHT", 0, "window height (defaults to 600)" },
	{"port", 'p', "PORT", 0, "listening port (if set to zero then only read from stdin - defaults to 5000)" },
	{"nogui",'g', 0, 0, "text-only interface (reads and broadcasts sgf)"},
	{"expand",'e', 0, 0, "expand sgf"},
	{"nomark",'m', 0, 0, "do not set mark after play"},
	{"swap",'s', 0, 0, "swap color"},
	{"rotate",'r', 0, 0, "random rotate"},
	{ 0 }
};
static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
	switch (key)
	{
		case 'w':
			if(!arg || 1!=sscanf(arg, "%d", &setts.width)) {
				perror("width must be an integer");
				exit(1);
			}
			break;
		case 'h':
			if(!arg || 1!=sscanf(arg, "%d", &setts.height)) {
				perror("height must be an integer");
				exit(1);
			}
			break;
		case 'p':
			if(!arg || 1!=sscanf(arg, "%d", &setts.port)) {
				perror("port must be an integer");
				exit(1);
			}
			break;
		case 'g':
			setts.nogui = 1;
			break;
		case 'e':
			setts.expand = 1;
			break;
		case 'm':
			setts.nomark = 1;
			break;
		case 's':
			setts.swap = 1;
			break;
		case 'r':
			setts.rotate = 1;
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}
static struct argp argp = { options, parse_opt, args_doc, doc };


int main(int argc, char **argv) {
	int s;
	struct sockaddr_in addr;

	memset(&setts,'\0', sizeof(setts));
	setts.width = 600;
	setts.height = 600;
	setts.port = 5000;

	argp_parse (&argp, argc, argv, 0, 0, 0);

	if(setts.port) {
		s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if(s < 0) {
			perror("socket()");
			return -1;
		}

		bzero(&addr, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.sin_port = htons(setts.port);

		int tr=1;
		if (setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&tr,sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if(0 > bind(s,(struct sockaddr*)&addr,sizeof(addr))) {
			perror("bind()");
			return -1;
		}

		if(0 > listen(s,1)) {
			perror("listen()");
			return -1;
		}
		Fl::add_fd(s, listen_cb, &s);
	}

	Fl::add_fd(STDIN_FILENO, read_cb, NULL);

	win = new Fl_Double_Window(setts.width,setts.height);
	win->begin();
	flgoban = new Fl_Goban(0,0,setts.width,setts.height, ev_key, ev_mou);
	//textbuffer = new Fl_Text_Buffer();
	//comment = new Fl_Text_Editor(10,650,WINDOW_WIDTH-20,90,0);
	//comment->buffer(textbuffer);
	win->end();
	win->resizable(flgoban);
	if(!setts.nogui)
		win->show();

	g = new_goban(19);
	memset(rd_cmd, '\0', sizeof(rd_cmd));
	rd = wr = rd_cmd;

	signal(SIGPIPE,handler_sigpipe);

	if(setts.nogui)
		while(1)
			Fl::check();
	else
		Fl::run();

	if(setts.port)
		close(s);
	goban_free(g);
}
