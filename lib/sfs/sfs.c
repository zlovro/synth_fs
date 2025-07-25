//
// Created by Made on 23/03/2025.
//

#include "sfs/sfs.h"

#include <serrno.h>
#include <stdlib.h>

#ifdef DESKTOP
FILE* gSfsFile;
#endif

u8 gSfsFirstBlockData[BLOCK_SIZE];
u8 gSfsTmpBlock[BLOCK_SIZE];

synthErrno sfsInit() {
    gSfsFile = fopen("C:/Users/Made/Documents/src/cxx/cross/synth-fs/workdir/synth.bin", "rb");
    if (!gSfsFile)
    {
        return SERR_SD_GENERIC_ERROR;
    }

    synthErrno ret = sfsReadBlockFull(gSfsFirstBlockData, 0);
    if (ret != SERR_OK)
    {
        return ret;
    }

    return SERR_OK;
}

synthErrno sfsDeinit() {
    fclose(gSfsFile);

    return SERR_OK;
}
