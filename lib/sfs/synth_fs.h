//
// Created by Made on 23/03/2025.
//

#ifndef SYNTH_FS_H
#define SYNTH_FS_H

#include <synth_errno.h>

#include "types.h"

#ifndef BLOCK_SIZE
#define BLOCK_SIZE 0x200
#endif

typedef pstruct
{
    u32 magic; // SYLZ

    u32 pcmDataBlockStart;
    u32 stringLutBlockStart;          // string lut - offsets into stringDataBlock
    u32 stringDataBlockStart;         // contigous!
    u32 instrumentInfoDataBlockStart; // big array of sfsSingleInstrument
    u32 sampleDataBlockStart;         // big array of sfsInstrumentSample

    u32 instrumentCount;

    u16 singleInstrumentCount;
    u16 multiInstrumentCount;
} sfsHeader;

typedef pstruct
{
    u16 pcmDataLengthBlocks;
    u32 pcmDataBlockOffset;
    u32 loopStart;
    u32 releaseStart;

    u16 fadeAmplitude;
    u16 loopDuration;
    u16 releaseDuration;

    u8  velocity;
    u8  pitchSemitones;

    u8 padding[10];
} sfsInstrumentSample;

assertSizeAlignedTo(sfsInstrumentSample, 0x200);

#define SFS_INVALID_SAMPLE_IDX 0xFFFF_FFFF

typedef pstruct
{
    u32 firstSampleBlock;
    // u32 externalReleaseSampleIdx; //SFS_INVALID_SAMPLE_IDX if no external sample (most common case)

    u16 nameStrIndex;

    u8 fadeTimeDefault; // fade time (seconds) = fadeTimeDefault / 16
    u8 fadeTimeForced;  // fade time (seconds) = fadeTimeForced / 16
    u8 noteRangeStart;
    u8 noteRangeEnd;
    u8 sampleBankCount;

    typedef enum : u8
    {
        SFS_SOUND_TYPE_ATTACK,
        SFS_SOUND_TYPE_ATTACK_LOOP,
        SFS_SOUND_TYPE_ATTACK_LOOP_RELEASE,
        SFS_SOUND_TYPE_ATTACK_RELEASE,
        SFS_SOUND_TYPE_LOOP,
        SFS_SOUND_TYPE_LOOP_RELEASE,
    } sfsSoundType;

    sfsSoundType soundType;

    u8 padding[20];
} sfsSingleInstrument;

assertSizeAlignedTo(sfsSingleInstrument, 0x200);

#define SFS_INVALID_INSTRUMENT_IDX 0xFF
#define SFS_MAX_SUBINSTRUMENTS 12

typedef pstruct
{
    u16 subInstrumentIds[SFS_MAX_SUBINSTRUMENTS];
    u16 nameStrIndex;

    u8 subInstrumentCount;
    u8 releaseInstrumentIdx; // if == SFS_INVALID_INSTRUMENT_IDX, ignore

    u8 padding[4];
} sfsMultiInstrument;

assertSize(sfsMultiInstrument, sizeof(sfsSingleInstrument));

extern synthErrno sfsReadBlocks(u8* pData, u32 pBlkIdx, u32 pBlkCnt);

#endif //SYNTH_FS_H
