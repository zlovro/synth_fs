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

#define SFS_SAMPLERATE 48000
#define SFS_SAMPLERATE_F32 ((f32)SFS_SAMPLERATE)

typedef pstruct
{
    u32 magic; // SYLZ

    u32 fontDataBlockStart;
    u32 holdBehaviorDataStart;
    u32 pcmDataBlockStart;
    u32 stringLutBlockStart;          // string lut - offsets into stringDataBlock
    u32 stringDataBlockStart;         // contigous!
    u32 instrumentInfoDataBlockStart; // big array of sfsSingleInstrument
    u32 sampleDataBlockStart;         // big array of sfsInstrumentSample

    u32 instrumentCount;

    u16 singleInstrumentCount;
    u16 multiInstrumentCount;
} sfsHeader;

#define SFS_MAGIC magic('S', 'Y', 'L', 'Z')

typedef pstruct
{
    u16 pcmDataLengthBlocks;
    u32 pcmDataBlockOffset;
    u32 loopStart;

    u16 loopDuration;

    u8 velocity;
    u8 pitchSemitones;

    u8 padding[18];
} sfsInstrumentSample;

assertSizeAlignedTo(sfsInstrumentSample, 0x200);

#define SFS_INVALID_SAMPLE_IDX 0xFFFF_FFFF

typedef enum : u8
{
    SFS_SOUND_TYPE_ATTACK = 1 << 0,
    SFS_SOUND_TYPE_LOOP   = 1 << 1,
} sfsSoundType;

enumAsFlag(sfsSoundType);

typedef pstruct
{
    u32 firstSampleBlock;

    u16 nameStrIndex;

    f32 fadeTimeDefault; // fade time (seconds) = fadeTimeDefault / 16
    f32 fadeTimeForced;  // fade time (seconds) = fadeTimeForced / 16
    u8  noteRangeStart;
    u8  noteRangeEnd;
    u8  sampleBankCount;

    sfsSoundType soundType;

    u8 padding[14];
} sfsSingleInstrument;

assertSizeAlignedTo(sfsSingleInstrument, 0x200);

#define SFS_INVALID_INSTRUMENT_IDX 0xFF
#define SFS_INVALID_INSTRUMENT_ID 0xFFFF
#define SFS_MAX_SUBINSTRUMENTS 12

typedef pstruct
{
    u16 subInstrumentIds[SFS_MAX_SUBINSTRUMENTS];
    u16 nameStrIndex;

    u8 subInstrumentCount;
    u8 chordInstrumentIdx; // if == SFS_INVALID_INSTRUMENT_IDX, ignore

    u8 padding[4];
} sfsMultiInstrument;

assertSize(sfsMultiInstrument, sizeof(sfsSingleInstrument));

typedef pstruct
{
    f32 triggerTime;
    f32 maxTriggerTime;
    f32 transitionTime;
    u16 instrumentId;

    u8 padding[2];
} sfsHoldBehaviour;

assertSizeAlignedTo(sfsHoldBehaviour, BLOCK_SIZE);

extern synthErrno sfsReadBlocks(u8* pData, u32 pBlkIdx, u32 pBlkCnt);

#endif //SYNTH_FS_H
