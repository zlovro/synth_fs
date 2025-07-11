//
// Created by Made on 23/03/2025.
//

#include "sfs/sfs.h"

#include <serrno.h>
#include <stdlib.h>

u8 gSfsFirstBlockData[BLOCK_SIZE];
u8 gSfsTmpBlock[BLOCK_SIZE];

synthErrno sfsInit() {
    synthErrno ret = sfsReadBlockFull(gSfsFirstBlockData, 0);
    if (ret != SERR_OK)
    {
        return ret;
    }

    return SERR_OK;
}

synthErrno sfsDeinit() {
    return SERR_OK;
}
