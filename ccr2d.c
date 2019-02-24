#include "ccr2d.h"
#include <signal.h>

void quicksort(sprite *spr, uint first, uint last)
{
	if (first < last)
	{
		sprite temp;
		uint pivot = first;
		uint i = first;
		uint j = last;
		while (i < j)
		{
			while (spr[i].pri<=spr[pivot].pri && i < last)
			{
				i++;
			}
			while (spr[j].pri > spr[pivot].pri)
			{
				j--;
			}
			if (i < j)
			{
				temp = spr[i];
				spr[i] = spr[j];
				spr[j] = temp;
			}
		}
		temp = spr[pivot];
		spr[pivot] = spr[j];
		spr[j] = temp;
		quicksort(spr, first, j - 1);
		quicksort(spr, j + 1, last);
	}
}

//background and sprites to pixels
TFUNC bs2p(void *vargp)
{
	ccr2d1 *obj = setup_thread(vargp);
	while (1)
	{
		pixel **pxl = pxlarr2dmallocxy(obj->wid, obj->hei);
		for (ulong y = 0; y < obj->hei; y++)
		{
			ulong i = y * obj->wid;
			for (ulong x = 0; x < obj->wid; x++)
			{
				pxl[x][y] = obj->bck[i + x];
			}
		}
		uint spc = obj->spc;
		sprite *spr = malloc(spc * sizeof(sprite));
		sprcpy(spr, obj->spr, spc);
		if(spc != 0)
			quicksort(spr, 0, spc - 1);
		for(uint i = 0; i < spc; i++)
		{
			sprite s = spr[i];
			for(ulong j = 0; j < s.hei; j++)
			{
				for(ulong k = 0; k < s.wid; k++)
				{
					pixel p =
						s.pxl[j * s.wid + k];
					//is not transparent
					if(p.dnsty != D_0)
					{
						pxl[(long long)
							k + s.x][
							(long long)j
								+ s.y]
							= p;
					}
				}
			}
		}
		for(ulong x = 0; x < obj->wid; x++)
			for(ulong y = 0; y < obj->hei; y++)
				obj->bfr.p[x][y] = pxl[x][y];
		pxlarr2dfreexy(pxl, obj->wid);
		free(spr);
		sleep_ms(obj->slp);
	}
}

//uint istrlen(int *i)
//{
//	uint j = 0;
//	while(i[j]) j++;
//	return j;
//}

//pixels to chars
TFUNC p2c(void *vargp)
{
	ccr2d1 *obj = setup_thread(vargp);
	while(1)
	{
		for(ulong y = 0; y < obj->hei; y++)
		{
			ulong j = obj->wid * 10;
			char *bfr = malloc(j);
			memset(bfr, 0, j);
			for(ulong x = 0; x < obj->wid; x++)
			{
				size_t i = strlen(bfr);
				pixel p = obj->bfr.p[x][y];
				memcpy(bfr + i, p.color, strlen(p.color));
				bfr[strlen(bfr)] = p.dnsty;
			}
			memcpy(obj->bfr.c[y], bfr, j);
			free(bfr);
		}
		sleep_ms(obj->slp);
	}
}

//chars to linear ints
TFUNC c2li(void *vargp)
{
	ccr2d1 *obj = setup_thread(vargp);
	while(1)
	{
		ulong j = 0;
		int *i = obj->bfr.i;
		char **c = obj->bfr.c;
		for(ulong y = 0; y < obj->hei; y++)
		{
			for(ulong x = 0; x < obj->wid; x++)
				i[j++] = c[y][x];
			i[j++] = '\r';
			i[j++] = '\n';
		}
		i[j] = '\0';
		sleep_ms(obj->slp);
	}
}

//linear ints to screen
TFUNC li2s(void *vargp)
{
	ccr2d1 *obj = setup_thread(vargp);
	while(1)
	{
#if WIN
		COORD c;
		c.X = c.Y = 0;
		SetConsoleCursorPosition(GetConsoleWindow(), c);
#else
		char *c = M_0_0;
		while (*c) putc(*c, stdout), c++;
#endif
		int *i = obj->bfr.i;
		ulong j = 0;
		while(i[j]) putc(i[j++], stdout);
		sleep_ms(obj->slp);
	}
}

CCR2D1_API uint c2dspradd(ccr2d1 *obj, int x, int y, uint pri,
	ulong wid, ulong hei, pixel *pxl)
{
	//setting the sprite first avoids toc-tou
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
	c2dsprmva(obj, sid,
		(obj->spr[sid].x + x) % obj->wid,
		(obj->spr[sid].y + y) % obj->hei);
}

CCR2D1_API void c2dsprmva(ccr2d1 *obj, uint sid, uint x, uint y)
{
	obj->spr[sid].x = x;
	obj->spr[sid].y = y;
}

//interrupt handler
void int_hdl(int i)
{
	if(i == SIGINT || i == SIGABRT || i == SIGTERM)
	{
#if !WIN
		if(system("stty cooked") == -1)
		{
			error_handler(ERR_SYSTEM_FAIL);
		}
#endif
		exit(0);
	}
	else if(i == SIGFPE) error_handler(ERR_SIGFPE);
	else if(i == SIGILL) error_handler(ERR_SIGILL);
	else error_handler(ERR_ILLSIG);
}

//key controller
TFUNC kc(void *vargp)
{
	ccr2d1 *obj = setup_thread(vargp);
	while(1)
	{
		int i =
#if WIN
		_getch();
#else
		getchar();
#endif
		if(i == 3) int_hdl(SIGINT);
		else for(uint j = 0; j<obj->klc; j++) obj->kel[j](i);
	}
}

CCR2D1_API ccr2d1 *c2dnew(pixel *bck, ulong wid, ulong hei,
	uint max_spr, uint slp, uint max_kel)
{
	ccr2d1 *obj = malloc(sizeof(ccr2d1));
	obj->wid = wid;
	obj->hei = hei;
	obj->run = 0;
	obj->bfr.c = malloc(hei * sizeof(char*));
	for(ulong i = 0; i < hei; i++)
	{
		obj->bfr.c[i] = malloc(wid * 10);
	}
	obj->bfr.p = malloc(wid * sizeof(pixel*));
	for(ulong i = 0; i < wid; i++)
	{
		obj->bfr.p[i] = malloc(hei * sizeof(pixel));
	}
	obj->bfr.i = malloc(wid * hei * 10 * sizeof(int) + hei * 2);
	obj->spr = malloc(sizeof(sprite) * max_spr);
	obj->spc = 0;
	ulong sz = wid * hei;
	obj->bck = malloc(sz * sizeof(pixel));
	pxlcpy(obj->bck, bck, sz);
	obj->slp = slp;
	obj->kel = malloc(max_kel * sizeof(void*));
	obj->klc = 0;
#if WIN
	//since Nov 2015 Windows 10 supports ASCII escape codes with this
	SetConsoleMode(GetConsoleWindow(),
		ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
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
	//this should clear but it doesnt
	//putc('\x0c', stdin);
	system("clear");
#if !WIN
	if(system("stty raw") == -1)
	{
		error_handler(ERR_SYSTEM_FAIL);
	}
#endif
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
	for(uint i = 0; i < obj->hei; i++)
	{
		free(obj->bfr.c[i]);
	}
	free(obj->bfr.c);
	for(uint i = 0; i < obj->wid; i++)
	{
		free(obj->bfr.p[i]);
	}
	free(obj->bfr.p);
	free(obj->kel);
	free(obj);
#if !WIN
	if(system("stty cooked") == -1)
	{
		error_handler(ERR_SYSTEM_FAIL);
	}
#endif
}

CCR2D1_API void pxlset(pixel *ptr, int dty, ulong num)
{
	for(ulong j = 0; j < num; j++)
	{
		ptr[j].dnsty = dty;
		ptr[j].color = C_NULL;
	}
}

CCR2D1_API void pxlcpy(pixel *dest, pixel *src, ulong n) {
	for(ulong i = 0; i < n; i++) dest[i] = src[i];
}

//void pxlarr(ulong len, pixel *bfr)
//{
//	for(ulong j = 0; j < len; j++)
//	{
//		bfr[j].dnsty = D_0;
//		bfr[j].color = C_NULL;
//	}
//}

CCR2D1_API void sprcpy(sprite *dest, sprite *src, ulong n) {
	for(ulong i = 0; i < n; i++) dest[i] = src[i];
}

CCR2D1_API void c2dkeladd(ccr2d1 *obj, kel ltr)
{
	obj->kel[obj->klc] = ltr;
	obj->klc++;
}

CCR2D1_API void c2dldp(FILE *stream, pixel *buffer, ulong *width, ulong *height)
{
	char hcorrect[8] = {'C', 'C', 'R', '2', 'D', '1', 'P', '\x01'};
	char hcheck[8];
	fread(hcheck, 1, 8, stream);
	if(strncmp(hcorrect, hcheck, 8))
		error_handler(ERR_INCORRECT_HEADER);
	char rwid[4];
	fread(rwid, 1, 4, stream);
	*width = SHIFTIN(rwid);
	char rhei[4];
	fread(rhei, 1, 4, stream);
	*height = SHIFTIN(rhei);
	ulong pc = *width * *height;
	for(ulong i = 0; i < pc; i++)
	{
		buffer[i].dnsty = fgetc(stream);
		int cl = fgetc(stream);
		buffer[i].color = malloc(cl);
		fread(buffer[i].color, 1, cl, stream);
	}
}

CCR2D1_API thread thread_create(tstart func, void *arg)
{
#if WIN
	return CreateThread(0, 0, func, arg, 0, 0);
#else
	pthread_t p;
	pthread_create(&p, 0, func, arg);
	return p;
#endif
}

CCR2D1_API void thread_cancel(thread t)
{
#if WIN
	TerminateThread(t, 0);
#else
	pthread_cancel(t);
#endif
}

CCR2D1_API ccr2d1 *setup_thread(void *arg)
{
#if !WIN
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0);
#endif
	return (ccr2d1*)arg;
}

CCR2D1_API pixel **pxlarr2dmallocxy(ulong wid, ulong hei)
{
	pixel **pxl = malloc(wid * sizeof(pixel*));
	for (uint i = 0; i < wid; i++)
	{
		pxl[i] = malloc(hei * sizeof(pixel));
	}
	return pxl;
}

CCR2D1_API void pxlarr2dfreexy(pixel **pxl, ulong wid)
{
	for (uint i = 0; i < wid; i++)
	{
		free(pxl[i]);
	}
	free(pxl);
}

CCR2D1_API void sleep_ms(uint ms)
{
#if WIN
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}
