/* AKAI OS backup reader - common header */

#ifndef _DEEPSTRIPPER_H
#define _DEEPSTRIPPER_H

extern int g_dbg;
#define _DBG(f,...)		{ if(g_dbg & (f)) g_print(__VA_ARGS__); }
#define _DBG_GUI		1
#define _DBG_PRJ		2
#define _DBG_BLK		4
#define _DBG_INT		8

// Project type, passed to akaiosproject_read() etc.
#define AOSP_DPS12		0
#define AOSP_DPS16		1

#endif
