/* In memory structure of an Akai OS Project backup */

#ifndef _AKAIOSSCENE_H
#define _AKAIOSSCENE_H

#define AOSS_DPS12_SIZE		0x400
#define AOSS_DPS16_SIZE		0xc00

typedef struct {
	char name[20];
	unsigned char pans[32];
	unsigned char levs[32];
} AkaiOsScene;

int akaiosscene_read(int fd, int type, AkaiOsScene *scn);

#endif
