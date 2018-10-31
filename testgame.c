#include "ccr2d.h"

#define WID 20
#define HEI 10
#define FPS 60
#define PXL WID * HEI

ccr2d1 *obj;

void key(int i)
{
	obj->spr[0].pxl[0].dnsty = i;
}

void err(int i)
{
	if(i == ERR_SYSTEM_FAIL)
	{
		puts("AAAAAHHHHH, WE BROKE SYSTEM");
	}
}

int main()
{
	error_handler = err;
	pixel bck[PXL];
	pxlarr(PXL, bck);
	obj = c2dnew(bck, WID, HEI, 10, 1000 / FPS, 10);
	c2dstart(obj);
	pixel spr[9];
	pxlarr(9, spr);
	spr[1].color = C_BLUE;
	spr[1].dnsty = D_2;
	spr[4].color = C_GREEN;
	spr[4].dnsty = D_1;
	c2dspradd(obj, 2, 2, 1, 3, 3, spr);
	c2dkeladd(obj, key);
	while(1) sleep_ms(1000000);
}
