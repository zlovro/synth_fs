//
// Created by Made on 23/03/2025.
//

#ifndef SYNTH_FS_H
#define SYNTH_FS_H

#include <serrno.h>
#include <solfege/solfege.h>
#include <types.h>

#ifndef BLOCK_SIZE
#define BLOCK_SIZE 0x200
#define SAMPLES_PER_BLOCK 0x100
#define SAMPLE_SIZE 2
#endif

#define roundToBlock(x) roundUpTo(x, BLOCK_SIZE)

#define SFS_SAMPLERATE 48000
#define SFS_SAMPLERATE_F32 ((f32)SFS_SAMPLERATE)

typedef pstruct {
    u32 magic; // SYLZ

    u32 fontDataBlockStart;
    u32 holdBehaviorDataStart;
    u32 pcmDataBlockStart;
    u32 stringLutBlockStart;          // string lut - offsets into stringDataBlock
    u32 stringDataBlockStart;         // contiguous!
    u32 instrumentInfoDataBlockStart; // big array of sfsSingleInstrument
    u32 sampleInfoBlockStart;         // big array of sfsInstrumentSample
    u32 proximityTableBlockStart;     // big array of proximityTables

    u32 instrumentCount;

    u16 singleInstrumentCount;
    u16 multiInstrumentCount;
} sfsHeader;

#define SFS_MAGIC magic('S', 'Y', 'L', 'Z')

typedef pstruct {
    u32 pcmDataLengthSamples;
    u32 pcmDataBlockOffset;
    u32 loopStart;
    u32 loopDuration;

    u8 velocity;
    u8 pitchSemitones;

    u16 startAverageAmplitude, endAverageAmplitude;
    u8  padding[10];
} sfsInstrumentSample;

assertSizeAlignedTo(sfsInstrumentSample, 0x200);
#define SAMPLE_INFOS_PER_BLOCK (BLOCK_SIZE / sizeof(sfsInstrumentSample))

#define SFS_INVALID_SAMPLE_IDX 0xFFFF_FFFF

typedef enum : u8 {
    SFS_SOUND_TYPE_ATTACK = 1 << 0,
    SFS_SOUND_TYPE_LOOP   = 1 << 1,
} sfsSoundType;

enumAsFlag(sfsSoundType)typedef pstruct {
    u16 nameStrIndex;

    f32 fadeTimeForced;
    u8  noteRangeStart;
    u8  noteRangeEnd;
    u8  sampleBankCount;

    sfsSoundType soundType;

    u8 padding[22];
} sfsSingleInstrument;

assertSizeAlignedTo(sfsSingleInstrument, 0x200);

#define SFS_INVALID_INSTRUMENT_IDX 0xFF
#define SFS_INVALID_INSTRUMENT_ID 0xFFFF
#define SFS_MAX_SUBINSTRUMENTS 12
#define SFS_MAX_VELOCITY_COUNT 8
#define SFS_INVALID_VELOCITY 0

#define SFS_FIRST_KEY TONE_OFFSET_C2
#define SFS_LAST_KEY TONE_OFFSET_C7
#define SFS_KEY_SEMITONE_RANGE (1 + SFS_LAST_KEY - SFS_FIRST_KEY)
#define SFS_KEY_COUNT SFS_KEY_SEMITONE_RANGE
#define SFS_KEYS_WORD_COUNT (SFS_KEY_SEMITONE_RANGE % 32 == 0 ? SFS_KEY_SEMITONE_RANGE / 32 : 1 + (SFS_KEY_SEMITONE_RANGE / 32))

typedef pstruct {
    u8  velocity;
    u16 sampleIdx;
    u8  padding;
} sfsKeyProximityTableEntryVelocity;

typedef pstruct {
    sfsKeyProximityTableEntryVelocity byVelocity[SFS_MAX_VELOCITY_COUNT];
} sfsKeyProximityTableEntryMaster;

assertSizeAlignedTo(sfsKeyProximityTableEntryMaster, BLOCK_SIZE);

typedef pstruct {
    u32                             sampleIdxOrigin;
    u8                              padding[sizeof(sfsKeyProximityTableEntryMaster) - 4];
    sfsKeyProximityTableEntryMaster masterEntries[SFS_KEY_COUNT];
    u8                              padding0[64];
} sfsKeyProximityTable;

static_assert(sizeof(sfsKeyProximityTable) % BLOCK_SIZE == 0);
#define SFS_PROXIMITY_TABLE_BLOCK_SIZE (sizeof(sfsKeyProximityTable) / BLOCK_SIZE)

typedef pstruct {
    u16 subInstrumentIds[SFS_MAX_SUBINSTRUMENTS];
    u16 nameStrIndex;

    u8 subInstrumentCount;
    u8 chordInstrumentIdx; // if == SFS_INVALID_INSTRUMENT_IDX, ignore

    u8 padding[4];
} sfsMultiInstrument;

assertSize(sfsMultiInstrument, sizeof(sfsSingleInstrument));

typedef pstruct {
    f32 triggerTime;
    f32 maxTriggerTime;
    f32 transitionTime;
    u16 instrumentId;

    u8 padding[2];
} sfsHoldBehaviour;

assertSizeAlignedTo(sfsHoldBehaviour, BLOCK_SIZE);

typedef pstruct {
    u8 drawingStartEnd; // first column that isnt empty
    // u8 drawingEnd   : 3; // last non empty column
    // u8 padding      : 2;
    u8 cols[8];
} sfsGlyph;

assertSize(sfsGlyph, 9);

#define SFS_GLYPH_COUNT 256
#define SFS_FONT_SIZE (SFS_GLYPH_COUNT * sizeof(sfsGlyph))
#define SFS_FONT_SIZE_ALIGNED (roundToBlock(SFS_GLYPH_COUNT * sizeof(sfsGlyph)))
#define SFS_FONT_SIZE_BLOCKS (SFS_FONT_SIZE_ALIGNED / BLOCK_SIZE)

extern u8 gSfsFirstBlockData[];
extern u8 gSfsTmpBlock[];

#ifdef DESKTOP

#include <stdio.h>

extern FILE* gSfsFile;

#endif

#define gSfsHeader ((sfsHeader*)gSfsFirstBlockData)

extern synthErrno sfsInit();
extern synthErrno sfsDeinit();
extern synthErrno sfsReadBlocks(u8 *pData, u32 pBlkIdx, u8 pBlkCnt, u16 pByteOffset, u16 pByteCount);
extern synthErrno sfsWriteBlocks(u8 *pData, u32 pBlkIdx, u8 pBlkCnt, u16 pByteOffset, u16 pByteCount);

#define sfsReadBlockFull(dat, blk) sfsReadBlocks(dat, blk, 1, 0, BLOCK_SIZE)
#define sfsReadBlocksFull(dat, blk, cnt) sfsReadBlocks(dat, blk, cnt, 0, BLOCK_SIZE)
#define sfsReadBlockFromOffsetToEnd(dat, blk, off) sfsReadBlocks(dat, blk, 1, off, BLOCK_SIZE - off)
#define sfsReadBlockFromOffsetPartial(dat, blk, off, count) sfsReadBlocks(dat, blk, 1, off, count)
#define sfsReadBlocksFromOffsetPartial(dat, blk, off, count) sfsReadBlocks(dat, blk, 1, off, count)

#define sfsWriteBlockFull(dat, blk) sfsWriteBlocks(dat, blk, 1, 0, BLOCK_SIZE)

#endif //SYNTH_FS_H
