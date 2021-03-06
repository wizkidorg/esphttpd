//Some quick-and-dirty GE G35 stuff for ESP8266

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Brady O'Brien <brady.obrien666@gmail.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include "g35.h"
#include "espmissingincludes.h"
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "stdlib.h"
#include <math.h>
#include <stdlib.h>

extern void wdt_feed(void);
extern void ets_wdt_disable(void);
extern void ets_wdt_enable(void);

static int g35_len = 0;

#define STRGPIO 2
#define DLY10US os_delay_us(10);
#define DLY20US os_delay_us(20);
#define LP_H gpio_output_set((1<<STRGPIO), 0, (1<<STRGPIO), 0);;
#define LP_L gpio_output_set(0, (1<<STRGPIO), (1<<STRGPIO), 0);
#define G35B_S { LP_H DLY10US }
#define G35B_0 { LP_L DLY10US LP_H DLY20US}
#define G35B_1 { LP_L DLY20US LP_H DLY10US }
#define G35B_E { LP_L DLY10US DLY10US DLY10US }

void g35_sendframe(char addr,char br,char r,char g,char b){
	
	G35B_S;
	if(addr & 0x20) G35B_1 else G35B_0
	if(addr & 0x10) G35B_1 else G35B_0
	if(addr & 0x08) G35B_1 else G35B_0
	if(addr & 0x04) G35B_1 else G35B_0
	if(addr & 0x02) G35B_1 else G35B_0
	if(addr & 0x01) G35B_1 else G35B_0

	if(br   & 0x80) G35B_1 else G35B_0
	if(br   & 0x40) G35B_1 else G35B_0
	if(br   & 0x20) G35B_1 else G35B_0
	if(br   & 0x10) G35B_1 else G35B_0
	if(br   & 0x08) G35B_1 else G35B_0
	if(br   & 0x04) G35B_1 else G35B_0
	if(br   & 0x02) G35B_1 else G35B_0
	if(br   & 0x01) G35B_1 else G35B_0

	if(b    & 0x08) G35B_1 else G35B_0
	if(b    & 0x04) G35B_1 else G35B_0
	if(b    & 0x02) G35B_1 else G35B_0
	if(b    & 0x01) G35B_1 else G35B_0

	if(g    & 0x08) G35B_1 else G35B_0
	if(g    & 0x04) G35B_1 else G35B_0
	if(g    & 0x02) G35B_1 else G35B_0
	if(g    & 0x01) G35B_1 else G35B_0

	if(r    & 0x08) G35B_1 else G35B_0
	if(r    & 0x04) G35B_1 else G35B_0
	if(r    & 0x02) G35B_1 else G35B_0
	if(r    & 0x01) G35B_1 else G35B_0
	
	G35B_E;
	G35B_E;
}

float flur(float in){
	int v = (int)in;
	return (float)v;
}
g35_color hsv_to_rgb( float h, float s, float v )
{
	g35_color color;
	float r,g,b;
	int i;
	float f, p, q, t;
	if( s == 0 ) {
		// achromatic (grey)
		color.r = (char)(r*15);
		color.g = (char)(g*15);
		color.b = (char)(b*15);
		r = g = b = v;
		return color;
	}
	h /= 60;			// sector 0 to 5
	i = flur( h );
	f = h - i;			// factorial part of h
	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );
	switch( i ) {
		case 0:
			r = v;
			g = t;
			b = p;
			break;
		case 1:
			r = q;
			g = v;
			b = p;
			break;
		case 2:
			r = p;
			g = v;
			b = t;
			break;
		case 3:
			r = p;
			g = q;
			b = v;
			break;
		case 4:
			r = t;
			g = p;
			b = v;
			break;
		default:		// case 5:
			r = v;
			g = p;
			b = q;
			break;
	}
	color.r = (char)(r*15);
	color.g = (char)(g*15);
	color.b = (char)(b*15);

	return color;
}

void g35_csendframe(char addr,char br,g35_color c){
	g35_sendframe(addr,br,c.r,c.g,c.b);
}

void g35_cfill(char br,g35_color c){
	int i;
	if(br<0)br=0;
	if(br>0x3F)br=0x3F;
	for(i=0;i<g35_len;i++){
		wdt_feed();
		g35_csendframe((char)i,br,c);
	}
	//g35_csendframe(0b111111,br,c);
}



void g35_pattern_blank(int t){
	return;
}

#define RED  {0xF,0x0,0x0}
#define BLUE {0x0,0x0,0xF}
#define GRN  {0x0,0xF,0x0}
#define WHT  {0xF,0xF,0xF}
#define BLK  {0x0,0x0,0x0}
#define YLW  {0xF,0xF,0x0}
#define VLO  {0xF,0x0,0xF}
#define CYAN {0x0,0xF,0xF}

g35_color numcol[]={WHT,RED,BLUE,GRN,YLW,VLO,CYAN,BLK};

static int wipeidx = 0;
static int wipeclr = 0;
void g35_pattern_wipefill(int tick){
	//if(tick%2==0){
		wipeidx++;
		if(wipeidx>g35_len){
			wipeidx=0;
			wipeclr++;
		}
		if(wipeclr>7)
			wipeclr=0;
		g35_csendframe((char)wipeidx,0x3f,numcol[wipeclr]);
			
	//}
}

static int strobe_st = 0;
void g35_pattern_whitestrobe(int tick){
	strobe_st = !strobe_st;
	g35_cfill(0x3F,numcol[strobe_st?0:7]);
}

void g35_pattern_cstrobe(int tick){
	g35_cfill(0x3F,numcol[tick%7]);
}

void g35_pattern_crotate(int tick){
	g35_cfill(0x3F,numcol[(tick/15)%6]);
}

void g35_pattern_fullrainbow(int tick){
	g35_color color = hsv_to_rgb((float)((tick*3)%360),1.0,1.0);
	g35_cfill(0x3F,color);
}

void g35_pattern_slidingrainbow(int tick){
	int i=0;
	for(i=0;i<g35_len;i++){
		g35_csendframe(i,0x3F,hsv_to_rgb((float)((tick*3+i*2)%360),1.0,1.0));
	}
}

static char fadepos=0;
static int fadedir=1;
static int fadeclr=0;
void g35_pattern_fade(int tick){
	if(fadedir==0)fadepos--;
	if(fadedir==1)fadepos++;
	if(fadepos<=0){fadedir=1;fadeclr++;}
	if(fadepos>=0x3F)fadedir=0;
	if(fadeclr>6)fadeclr=0;
	g35_cfill(fadepos,numcol[fadeclr]);
}

static int pong_ccol = 0;
static int pong_cpos = 0;
static int pong_dir = 1;
void g35_pattern_pingpong(int tick){
	g35_csendframe(pong_cpos,0x3F,numcol[pong_ccol]);
	pong_cpos += pong_dir;
	if(pong_cpos>g35_len){
		pong_cpos--;
		pong_dir=-1;
		pong_ccol++;
	}if(pong_cpos<0){
		pong_cpos++;
		pong_dir=1;
		pong_ccol++;
	}if(pong_ccol>7)pong_ccol=0;
	g35_csendframe(pong_cpos,0x3F,numcol[0]);
}

void g35_pattern_fslidingcolors(int tick){
	int i=0;
	for(i=0;i<g35_len;i++){
		g35_csendframe(i,0x3F,numcol[((i>>2)+tick)%7]);
	}
}

void g35_pattern_slidingcolors(int tick){
	int i=0;
	for(i=0;i<g35_len;i++){
		g35_csendframe(i,0x3F,numcol[((i+tick/2)>>2)%7]);
	}
}

static int chasercount = 5;
static int chaseri[20] = {1,2,7,10,13,24,30};
static int chaserv[20] = {1,4,-2,-5,-3,6};
static g35_color chaserc[20] = {RED,GRN,WHT,BLUE,YLW,CYAN};

void g35_init_chasers(){
	srand(0);
	rand();
}

void g35_ps_chasers(int tick,int ct){
	int i;
	if(tick%2) return;
	if(ct)
		g35_csendframe(63,0x00,numcol[7]);	//Clear current spot
	for(i=0;i<chasercount;i++){
		chaseri[i]+=chaserv[i];
		if(chaseri[i]>g35_len)chaseri[i]-=g35_len;
		g35_csendframe((char)chaseri[i],0x3F,chaserc[i]);
	}
}

void g35_pattern_chasers_f(int tick){g35_ps_chasers(tick,0);}
void g35_pattern_chasers_c(int tick){g35_ps_chasers(tick,1);}

int g35_pattern_count = 13;
g35_pattern g35_patterns[] = {
	{g35_pattern_blank,0,"Blank"},
	{g35_pattern_wipefill,1,"Wipe"},
	{g35_pattern_pingpong,1,"Ping-Pong"},
	{g35_pattern_fade,1,"Fade"},
	{g35_pattern_chasers_f,1,"Chasers"},
	{g35_pattern_chasers_c,1,"Point Chasers"},
	{g35_pattern_crotate,1,"Color Rotate"},
	{g35_pattern_fullrainbow,1,"Smooth Rainbow"},
	{g35_pattern_slidingrainbow,1,"Sliding Rainbow"},
	{g35_pattern_fslidingcolors,1,"Color Party"},
	{g35_pattern_slidingcolors,1,"Sliding Colors"},
	{g35_pattern_whitestrobe,0,"Strobe"},
	{g35_pattern_cstrobe,0,"Color Strobe"},
	{NULL,0,""}
};

int g35_get_pattern_count(){
	return g35_pattern_count;
}

static int tick_count = 0;
static int cur_pattern = 1;
static int rotate_patterns = 1;
static int pattern_rotate_ticks = 800;

void g35_tick(void){
	wdt_feed();
	ets_wdt_disable();
	tick_count++;
	if(tick_count > pattern_rotate_ticks && rotate_patterns){
		tick_count = 0;
		int old_pattern = cur_pattern;
		
		cur_pattern++;
		while(!(g35_patterns[cur_pattern].enabled)&&(cur_pattern!=old_pattern)){
			cur_pattern++;
			if(g35_patterns[cur_pattern].g35_pattern_ticker==NULL)
				cur_pattern=0;
		}
	}
	g35_patterns[cur_pattern].g35_pattern_ticker(tick_count);
	ets_wdt_enable();
}

static void ICACHE_FLASH_ATTR tick_timer_cb(void *arg){
	g35_tick();
}

/*
* Set the current pattern
*/
void set_cur_pattern(int pattern){
	int i=0;
	while(g35_patterns[i].g35_pattern_ticker) i++; //find the last pattern in the list
	if(pattern<i) cur_pattern=pattern;
}

int get_cur_pattern(){
	return cur_pattern;
}

void set_rotate_pattern(int rotate){
	rotate_patterns=rotate;
}

g35_pattern * get_patterns(){
	return g35_patterns;
}

static ETSTimer pattern_tick_timer;
void g35_init(int len){
	int i;
	g35_len = len;
	for(i=0;i<len;i++){
		g35_sendframe((char)i,0x3F,0xF,0xF,0xF);
		os_delay_us(10000);
	}
	os_timer_disarm(&pattern_tick_timer);
	os_timer_setfn(&pattern_tick_timer, tick_timer_cb, NULL);
	os_timer_arm(&pattern_tick_timer,40, 1);
}










