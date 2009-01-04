#ifndef _SCACHE_H
#define _SCACHE_H 1

int  scache_init(int devId, int sectorSize);
void scache_close(int devId);
void scache_kill(int devId); //dlanor: added for disconnection events (flush impossible)
int  scache_allocSector(int devId, unsigned int sector, void** buf);
int  scache_readSector(int devId, unsigned int sector, void** buf);
int  scache_writeSector(int devId, unsigned int sector);
int  scache_flushSectors(int devId);

void scache_getStat(int devId, unsigned int* access, unsigned int* hits);

#endif 
