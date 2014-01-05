/* AKAI OS backup reader for scene settings */

#include <string.h>
#ifdef WIN32
#include <io.h>
#include <stdlib.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif
#include "deepstripper.h"
#include "akaiosscene.h"
#include "diskbuffer.h"

int akaiosscene_read(int fd, int type, AkaiOsScene *scn) {
	// read scene block
	char buf[AOSS_DPS16_SIZE];
	int len = AOSP_DPS12==type ? AOSS_DPS12_SIZE : AOSS_DPS16_SIZE;
	if (read_buffered(fd, buf, len)!=len)
		return -1;
	// copy out name
	memcpy(scn->name, buf, 16);
	scn->name[16]=0;
	// copy out scene pans/levels
	memcpy(scn->pans, &buf[16], sizeof(scn->pans));
	memcpy(scn->levs, &buf[16+sizeof(scn->pans)], sizeof(scn->levs));
	return 0;
}
