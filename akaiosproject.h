/* In memory structure of an Akai OS Project backup */

#ifndef _AKAIOSPROJECT_H
#define _AKAIOSPROJECT_H

#include "akaiosscene.h"
#include "akaiosvtrack.h"

typedef struct {
	unsigned short offset;	// segment data offset (bytes) from header start
	unsigned short scnlen;	// length of additional scene data (bytes)
	int splrate;			// sample rate (sample/sec)
	int splsize;			// sample size (bits)
	int splbyte;			// sample storage size (bytes)
	char name[20];			// project name (null terminated)
	unsigned int memin;		// memory in point (sample offset)
	unsigned int memout;	// memory out point (sample offset)
	AkaiOsScene defscene;	// default scene
	AkaiOsScene *scenes;	// additional scenes..
	AkaiOsVTrack *vtracks;	// virtual tracks..
} AkaiOsProject;

int akaiosproject_read(int fd, int type, AkaiOsProject *proj);
void akaiosproject_clear(AkaiOsProject *proj);
void akaiosproject_tracks(AkaiOsProject *proj, int (*cb)(char *, void *), void *);
void akaiosproject_mixes(AkaiOsProject *proj, int (*cb)(char *, void *), void *);
void akaiosproject_memory(AkaiOsProject *proj, int (*cb)(char *, void *), void *);
AkaiOsVTrack *akaiosproject_track(AkaiOsProject *proj, char *name);

#endif
