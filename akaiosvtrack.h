/* AKAI OS project reader, virtual track data */
#ifndef _AKAIOSVTRACK_H
#define _AKAIOSVTRACK_H

typedef struct _aoss_seg {
	unsigned int start;	// segment start (sample offset)
	unsigned int end;		// segment end (sample offset)
	unsigned int offset;	// segment data offset (bytes) from seg data start
	struct _aoss_seg *next;	// chain of segments
} AkaiOsSegment;

typedef struct {
	char name[20];	// Track name
	int channel;	// assigned physical channel (1-12/16)
	AkaiOsSegment *segs;	// data segments chain
} AkaiOsVTrack;

#endif
