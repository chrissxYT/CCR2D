#include "ccr2d.h"
#include <signal.h>

void quicksort(sprite *spr, uint first, uint last)
{
	if (first < last)
	{
		sprite temp;
		uint p = first;
		uint i = first;
		uint j = last;
		while (i < j)
		{
			while(spr[i].pri<=spr[p].pri && i < last) i++;
			while(spr[j].pri > spr[p].pri) j--;
			if (i < j)
			{
				temp = spr[i];
				spr[i] = spr[j];
				spr[j] = temp;
			}
		}
		temp = spr[p];
		spr[p] = spr[j];
		spr[j] = temp;
		quicksort(spr, first, j - 1);
		quicksort(spr, j + 1, last);
	}
}

//background and sprites to pixels
TFUNC bs2p(void *vargp)
{
	setup_thread(vargp);
	while (1)
	{
		pixel **pxl = pxlarr2dmallocxy(obj->wid, obj->hei);
		for (ulong y = 0; y < obj->hei; y++)
		{
			ulong i = y * obj->wid;
			for (ulong x = 0; x < obj->wid; x++)
				pxl[x][y] = obj->bck[i + x];
		}
		uint spc = obj->spc;
		sprite *spr = malloc(spc * sizeof(sprite));
		sprcpy(spr, obj->spr, spc);
		if(spc > 0) quicksort(spr, 0, spc - 1);
		for(uint i = 0; i < spc; i++)
		{
			sprite s = spr[i];
			for(ulong j = 0; j < s.hei; j++)
				for(ulong k = 0; k < s.wid; k++)
				{
					pixel p =
						s.pxl[j * s.wid + k];
					//is not transparent
					if(strcmp(p.dnsty, D_0))
						pxl[k + s.x][
							j + s.y]
							= p;
				}
		}
		for (ulong x = 0; x < obj->wid; x++)
			memcpy(obj->bfr.p[x], pxl[x], obj->hei);
		pxlarr2dfreexy(pxl, obj->wid);
		free(spr);
		sleep_ms(obj->slp);
	}
}

//pixels to chars
TFUNC p2c(void *vargp)
{
	setup_thread(vargp);
	while(1)
	{
		for(ulong y = 0; y < obj->hei; y++)
		{
			ulong j = obj->wid * 256;
			char *bfr = malloc(j);
			char *bfs = bfr;
			for(ulong x = 0; x < obj->wid; x++)
			{
				pixel p = obj->bfr.p[x][y];
				char t[256];
				char *s = t;
				sprintf(s, "\x1b[38;2;%d;%d;%dm", p.r, p.g, p.b);
				while(*s) *bfr++ = *s++;
				s = p.dnsty;
				while(*s) *bfr++ = *s++;
			}
			*bfr = '\0';
			memcpy(obj->bfr.c[y], bfs, j);
			free(bfs);
		}
		sleep_ms(obj->slp);
	}
}

//chars to linear ints
TFUNC c2li(void *vargp)
{
	setup_thread(vargp);
	while(1)
	{
		int *i = obj->bfr.i;
		char **c = obj->bfr.c;
		for(ulong y = 0; y < obj->hei; y++)
		{
			char *d = c[y];
			for(ulong x = 0; x < obj->wid; x++)
				*i++ = d[x];
			*i++ = '\r';
			*i++ = '\n';
		}
		*i = '\0';
		sleep_ms(obj->slp);
	}
}

//linear ints to screen
TFUNC li2s(void *vargp)
{
	setup_thread(vargp);
	while(1)
	{
		M_0_0();
		int *i = obj->bfr.i;
		while(*i) putc(*i++, stdout);
		sleep_ms(obj->slp);
	}
}

CCR2D1_API uint c2dspradd(ccr2d1 *obj, uint x, uint y, uint pri,
	uint wid, uint hei, pixel *pxl)
{
	//setting the sprite first avoids ToC/ToU
	obj->spr[obj->spc].pri = pri;
	obj->spr[obj->spc].wid = wid;
	obj->spr[obj->spc].hei = hei;
	obj->spr[obj->spc].pxl = pxl;
	obj->spr[obj->spc].x = x;
	obj->spr[obj->spc].y = y;
	obj->spc++;
	return obj->spc - 1;
}

CCR2D1_API void c2dsprmvr(ccr2d1 *obj, uint sid, uint x, uint y)
{
	uint xx = obj->spr[sid].x + x;
	uint yy = obj->spr[sid].y + y;
	c2dsprmva(obj, sid,
		xx + obj->spr[sid].wid < obj->wid ? xx : obj->wid - 1,
		yy + obj->spr[sid].hei < obj->hei ? yy : obj->hei - 1);
}

CCR2D1_API colpos c2dchkcol(ccr2d1 *obj, uint sid1, uint sid2)
{
	sprite s1 = obj->spr[sid1];
	sprite s2 = obj->spr[sid2];
	uint x1 = s1.x;
	uint y1 = s1.y;
	uint x2 = s2.x;
	uint y2 = s2.y;
	uint x1w = x1 + s1.wid;
	uint y1h = y1 + s1.hei;
	uint x2w = x2 + s2.wid;
	uint y2h = y2 + s2.hei;
	bool x1c  = x1  >= x2 && x1  < x2w;
	bool y1c  = y1  >= y2 && y1  < y2h;
	bool x1wc = x1w >= x2 && x1w < x2w;
	bool y1hc = y1h >= y2 && y1h < y2h;
	bool x2c  = x2 >= x1 && x2 < x1w;
	bool y2c  = y2 >= y1 && y2 < y1h;
	bool x2wc = x2w >= x1 && x2w < x1w;
	bool y2hc = y2h >= y1 && y2h < y1h;
	return (x1c  && y1c ) ? TL1 :
	       (x1wc && y1c ) ? TR1 :
	       (x1c  && y1hc) ? BL1 :
	       (x1wc && y1hc) ? BR1 :
	       (x2c  && y2c ) ? TL2 :
	       (x2wc && y2c ) ? TR2 :
	       (x2c  && y2hc) ? BL2 :
	       (x2wc && y2hc) ? BR2 :
	       NONE;
}

//interrupt handler
void int_hdl(int i)
{
	if(i == SIGINT || i == SIGABRT || i == SIGTERM)
	{
		unix
		(
			if(system("stty cooked") == -1)
				error_handler(ERR_SYSTEM_FAIL);
		)
		exit(0);
	}
	else error_handler(
		i == SIGFPE ? ERR_SIGFPE :
		i == SIGILL ? ERR_SIGILL :
		ERR_ILLSIG);
}

//key controller
TFUNC kc(void *vargp)
{
	setup_thread(vargp);
	while(1)
	{
		int i = _PASSIVE_READ();
		if(i == 3) int_hdl(SIGINT);
		else for(uint j = 0; j<obj->klc; j++) obj->kel[j](i);
	}
}

CCR2D1_API ccr2d1 *c2dnew(pixel *bck, uint wid, uint hei,
	uint max_spr, uint slp, uint max_kel)
{
	ccr2d1 *obj = malloc(sizeof(ccr2d1));
	obj->wid = wid;
	obj->hei = hei;
	obj->run = 0;
	obj->bfr.c = malloc(hei * sizeof(char*));
	for(ulong i = 0; i < hei; i++) obj->bfr.c[i] = malloc(wid*256);
	obj->bfr.p = malloc(wid * sizeof(pixel*));
	for(ulong i = 0; i < wid; i++)
		obj->bfr.p[i] = malloc(hei * sizeof(pixel)),
			pxlset(obj->bfr.p[i], D_E, hei);
	obj->bfr.i = malloc(wid * hei * 10 * sizeof(int) + hei * 2);
	obj->spr = malloc(sizeof(sprite) * max_spr);
	obj->spc = 0;
	ulong sz = wid * hei;
	obj->bck = malloc(sz * sizeof(pixel));
	pxlcpy(obj->bck, bck, sz);
	obj->slp = slp;
	obj->kel = malloc(max_kel * sizeof(void*));
	obj->klc = 0;
	return obj;
}

CCR2D1_API void c2dstart(ccr2d1 *obj)
{
	signal(SIGABRT, int_hdl);
	signal(SIGFPE, int_hdl);
	signal(SIGILL, int_hdl);
	signal(SIGINT, int_hdl);
	//asan does this
	//signal(SIGSEGV, int_hdl);
	signal(SIGTERM, int_hdl);
	//this should clear on unix but it doesnt
	//putc('\x0c', stdin);
	system(win("cls") unix("clear"));
	//since Nov 2015 Windows 10 supports ASCII escape codes with this
	win(SetConsoleMode(GetConsoleWindow(),
		ENABLE_VIRTUAL_TERMINAL_PROCESSING));
	unix
	(
		if(system("stty raw") == -1)
			error_handler(ERR_SYSTEM_FAIL);
	)
	obj->run = 1;
	obj->wkr[0] = thread_create(bs2p, obj);
	obj->wkr[1] = thread_create(p2c, obj);
	obj->wkr[2] = thread_create(c2li, obj);
	obj->wkr[3] = thread_create(li2s, obj);
	obj->wkr[4] = thread_create(kc, obj);
}

CCR2D1_API void c2dstop(ccr2d1 *obj)
{
	obj->run = 0;
	thread_cancel(obj->wkr[0]);
	thread_cancel(obj->wkr[1]);
	thread_cancel(obj->wkr[2]);
	thread_cancel(obj->wkr[3]);
	thread_cancel(obj->wkr[4]);
	for (uint i = 0; i < obj->hei; i++)
		free(obj->bfr.c[i]);
	free(obj->bfr.c);
	for (uint i = 0; i < obj->wid; i++)
		free(obj->bfr.p[i]);
	free(obj->bfr.p);
	free(obj->kel);
	free(obj);
	unix
	(
		if(system("stty cooked") == -1)
			error_handler(ERR_SYSTEM_FAIL);
	)
	win(_setmode(_fileno(stdout), _O_U8TEXT));
}

CCR2D1_API void pxlset(pixel *ptr, str dty, ulong num)
{
	for(ulong j = 0; j < num; j++)
	{
		strcpy(ptr[j].dnsty, dty);
		ptr[j].r = ptr[j].g = ptr[j].b = 0xff;
	}
}

CCR2D1_API void c2dkeladd(ccr2d1 *obj, kel ltr)
{
	obj->kel[obj->klc] = ltr;
	obj->klc++;
}

CCR2D1_API void c2dldp(FILE *stream, pixel *buffer, uint *width, uint *height)
{
	char hcorrect[8] = {'f', 'a', 'r', 'b', 'f', 'e', 'l', 'd'};
	char hcheck[8];
	if(!fread(hcheck, 1, 8, stream))
		error_handler(ERR_FREAD_FAIL);
	if(strncmp(hcorrect, hcheck, 8))
		error_handler(ERR_INCORRECT_HEADER);
	char rwid[4];
	if(!fread(rwid, 1, 4, stream))
		error_handler(ERR_FREAD_FAIL);
	*width = BESHIFTIN32(rwid);
	char rhei[4];
	if(!fread(rhei, 1, 4, stream))
		error_handler(ERR_FREAD_FAIL);
	*height = BESHIFTIN32(rhei);
	ulong pc = *width * *height;
	for(ulong i = 0; i < pc; i++)
	{
		char c[8];
		if(!fread(c, 1, 8, stream))
			error_handler(ERR_FREAD_FAIL);
		buffer[i].r = BESHIFTIN16(c+0);
		buffer[i].g = BESHIFTIN16(c+2);
		buffer[i].b = BESHIFTIN16(c+4);
                buffer[i].dnsty[0] = c[6];
                buffer[i].dnsty[1] = c[7];
                buffer[i].dnsty[2] = '\0';
                // run a user supplied function to convert the raw
                // alpha vals to chars maybe
	}
}

CCR2D1_API void c2ddefaultalpha2dnsty(unsigned short alpha, char *dnsty)
{
             if(alpha == 0)
                strcpy(D_0, dnsty);
        else if(alpha == 100)
                strcpy(D_1, dnsty);
        else if(alpha == 200)
                strcpy(D_2, dnsty);
        else if(alpha == 300)
                strcpy(D_3, dnsty);
        else if(alpha == 400)
                strcpy(D_4, dnsty);
        else
                strcpy(D_E, dnsty);
}

CCR2D1_API thread thread_create(tstart func, void *arg)
{
	win(return CreateThread(0, 0, func, arg, 0, 0));
	unix (
		pthread_t p;
		pthread_create(&p, 0, func, arg);
		return p;
	)
}

CCR2D1_API pixel **pxlarr2dmallocxy(uint wid, uint hei)
{
	pixel **pxl = malloc(wid * sizeof(pixel*));
	for (uint i = 0; i < wid; i++)
		pxl[i] = malloc(hei * sizeof(pixel));
	return pxl;
}

CCR2D1_API void pxlarr2dfreexy(pixel **pxl, uint wid)
{
	for (uint i = 0; i < wid; i++)
		free(pxl[i]);
	free(pxl);
}
