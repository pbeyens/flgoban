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

static Fl_Double_Window *win = NULL;
static Fl_Goban *flgoban = NULL;
//static Fl_Text_Buffer *textbuffer = NULL;
//static Fl_Text_Editor *comment = NULL;
static list<int> fds;
static struct goban *g = NULL;
static char cx_prev = -1, cy_prev = -1;
static char broadcast_msg[10000];

struct settings
{
	int width, height, port, nogui, repeat;
};
static struct settings setts;

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
	printf("%s",cmd);
}

static void read_cb(int fd, void *data)
{
	int n;
	char rd_cmd[10000];
	memset(rd_cmd, '\0', sizeof(rd_cmd));
	memset(broadcast_msg, '\0', sizeof(rd_cmd));
	n = read(fd, rd_cmd, sizeof(rd_cmd));
	if(n <= 0) {
		Fl::remove_fd(fd);
		fds.remove(fd);
		if(setts.nogui && !fds.size())
			exit(0);
		return;
	}
	//printf("Received: %s\n",rd_cmd);
	sgf_parse_fast(rd_cmd);
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

/* GOBAN */
static void goban_ab(struct goban *gob, int x, int y)
{
	char msg[10];
	flgoban->set_stone(x,y,black);
	if(!setts.repeat) {
		sprintf(msg, "AB[%c%c]", int2sgf(x),int2sgf(y));
		strcat(broadcast_msg, msg);
	}
}

static void goban_aw(struct goban *gob, int x, int y)
{
	char msg[10];
	flgoban->set_stone(x,y,white);
	if(!setts.repeat) {
		sprintf(msg, "AW[%c%c]", int2sgf(x),int2sgf(y));
		strcat(broadcast_msg, msg);
	}
}

static void goban_ae(struct goban *gob, int x, int y)
{
	char msg[10];
	flgoban->set_stone(x,y,empty);
	if(!setts.repeat) {
		sprintf(msg, "AE[%c%c]", int2sgf(x),int2sgf(y));
		strcat(broadcast_msg, msg);
	}
}

static const struct goban_cb gcb = { goban_ab, goban_aw, goban_ae };

/* SGF */
static void sgf_node(void)
{
	strcat(broadcast_msg, ";\n");
}

static void sgf_sz(int s)
{
	char msg[10];
	goban_free(g);
	g = goban_alloc(s, &gcb);
	flgoban->flresize(s);
	cx_prev = -1, cy_prev = -1;
	sprintf(msg, "SZ[%d]", s);
	strcat(broadcast_msg, msg);
}

static void sgf_b(char cx, char cy)
{
	char msg[20];
	if(cx_prev!=-1) {
		flgoban->set_mark(sgf2int(cx_prev),sgf2int(cy_prev),empty);
		if(!setts.repeat) {
			sprintf(msg, "ME[%c%c]", cx_prev,cy_prev);
			strcat(broadcast_msg, msg);
		}
	}
	goban_play(g, sgf2int(cx), sgf2int(cy), black);
	flgoban->set_mark(sgf2int(cx),sgf2int(cy),circle);

	if(setts.repeat) {
		sprintf(msg, "B[%c%c]", cx,cy);
		strcat(broadcast_msg, msg);
	}
	else {
		sprintf(msg, "CR[%c%c]", cx, cy);
		strcat(broadcast_msg, msg);
	}

	cx_prev = cx;
	cy_prev = cy;
}

static void sgf_w(char cx, char cy)
{
	char msg[20];
	if(cx_prev!=-1) {
		flgoban->set_mark(sgf2int(cx_prev),sgf2int(cy_prev),empty);
		if(!setts.repeat) {
			sprintf(msg, "ME[%c%c]", cx_prev,cy_prev);
			strcat(broadcast_msg, msg);
		}
	}

	goban_play(g, sgf2int(cx), sgf2int(cy), white);
	flgoban->set_mark(sgf2int(cx),sgf2int(cy),circle);

	if(setts.repeat) {
		sprintf(msg, "W[%c%c]", cx,cy);
		strcat(broadcast_msg, msg);
	}
	else {
		sprintf(msg, "CR[%c%c]", cx, cy);
		strcat(broadcast_msg, msg);
	}

	cx_prev = cx;
	cy_prev = cy;
}

static void sgf_ab(char cx, char cy)
{
	char msg[10];
	goban_set(g, sgf2int(cx), sgf2int(cy), black);
	if(setts.repeat) {
		sprintf(msg, "AB[%c%c]", cx,cy);
		strcat(broadcast_msg, msg);
	}
}

static void sgf_aw(char cx, char cy)
{
	char msg[10];
	goban_set(g, sgf2int(cx), sgf2int(cy), white);
	if(setts.repeat) {
		sprintf(msg, "AW[%c%c]", cx,cy);
		strcat(broadcast_msg, msg);
	}
}

static void sgf_ae(char cx, char cy)
{
	char msg[10];
	goban_set(g, sgf2int(cx), sgf2int(cy), empty);
	if(setts.repeat) {
		sprintf(msg, "AE[%c%c]", cx,cy);
		strcat(broadcast_msg, msg);
	}
}

static const struct sgf_cb scb = { sgf_node, sgf_sz, sgf_b, sgf_w, sgf_ab, sgf_aw, sgf_ae };

/* EVENTS */

static void ev_key(char k)
{
	char e[100];
	sprintf(e, "KEY[%c];\n", k);
	broadcast(e);
}


/* ARGP */
const char *argp_program_version = "flgoban 0.1";
const char *argp_program_bug_address = "<pieter.beyens@gmail.com>";
static char doc[] = "flgoban -- reads, displays and broadcasts sgf";
static char args_doc[] = "";
static struct argp_option options[] = {
	{"width", 'w', "WIDTH", 0, "window width (defaults to 800)" },
	{"height", 'h', "HEIGHT", 0, "window height (defaults to 800)" },
	{"port", 'p', "PORT", 0, "listening port (if set to zero then only read from stdin - defaults to 5000)" },
	{"nogui",'n', 0, 0, "text-only interface (reads and broadcasts sgf)"},
	{"repeat",'r', 0, 0, "do not expand sgf, just repeat it"},
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
		case 'n':
			setts.nogui = 1;
			break;
		case 'r':
			setts.repeat = 1;
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
	flgoban = new Fl_Goban(0,0,setts.width,setts.height, ev_key);
	//textbuffer = new Fl_Text_Buffer();
	//comment = new Fl_Text_Editor(10,650,WINDOW_WIDTH-20,90,0);
	//comment->buffer(textbuffer);
	win->end();
	win->resizable(flgoban);
	if(!setts.nogui)
		win->show(argc, argv);

	g = goban_alloc(19, &gcb);
	sgf_init(&scb);

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
