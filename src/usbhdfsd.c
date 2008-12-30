/*
 * usb_mass.c - USB Mass storage driver for PS2
 */

#define MAJOR_VER 1
#define MINOR_VER 1

#include <loadcore.h>
#include <stdio.h>

extern int InitFAT();
extern int InitCache();
extern int InitFS();
extern int InitUSB();

int _start( int argc, char **argv)
{
	printf("USB HDD FileSystem Driver v%d.%d\n", MAJOR_VER, MINOR_VER);

	FlushDcache();

    // initialize the FAT driver
    if(InitFAT() != 0)
    {
        printf("Error initializing FAT driver!\n");
        return(1);
    }

    // initialize the Cache
    if(InitCache() != 0)
    {
        printf("Error initializing Cache!\n");
        return(1);
    }

    // initialize the USB driver
	if(InitUSB() != 0)
    {
        printf("Error initializing USB driver!\n");
        return(1);
    }

    // initialize the file system driver	    
    if(InitFS() != 0)
    {
        printf("Error initializing FS driver!\n");
        return(1);
    }

    // return resident
    return(0);
}
