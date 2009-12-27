/* AKAI OS backup reader utility functions */

#include "akaiosutil.h"

unsigned int be2hl(unsigned int b) {
	unsigned char *p = (unsigned char *)&b;
	return (unsigned int)((p[0]<<24) | (p[1]<<16) | (p[2]<<8) | p[3]);
}

unsigned short be2hs(unsigned short b) {
	unsigned char *p = (unsigned char *)&b;
	return (unsigned short)((p[0]<<8) | p[1]);
}

unsigned int h2bel(unsigned int h) {
	unsigned int r;
	unsigned char *p = (unsigned char *)&r;
	p[0] = (unsigned char)(h>>24);
	p[1] = (unsigned char)(h>>16);
	p[2] = (unsigned char)(h>>8);
	p[3] = (unsigned char)h;
	return r;
}

unsigned short h2bes(unsigned short h) {
	unsigned short r;
	unsigned char *p = (unsigned char *)&r;
	p[0] = (unsigned char)(h>>8);
	p[1] = (unsigned char)h;
	return r;
}
