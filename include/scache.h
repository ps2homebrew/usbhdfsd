#ifndef _SCACHE_H
#define _SCACHE_H 1

int  scache_init(int device, int sectorSize);
void scache_close(int device);
void scache_kill(int device); //dlanor: added for disconnection events (flush impossible)
int  scache_allocSector(int device, unsigned int sector, void** buf);
int  scache_readSector(int device, unsigned int sector, void** buf);
int  scache_writeSector(int device, unsigned int sector);
int  scache_flushSectors(int device);

void scache_getStat(int device, unsigned int* access, unsigned int* hits);

#endif 
