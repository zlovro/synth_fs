//
// Created by Made on 26/01/2025.
//

#ifndef WAV_H
#define WAV_H

#include <types.h>
#include <fatfs/ff.h>

#define WAV_SAMPLE_RATE 48000
#define WAV_SAMPLE_SIZE_BITS 16
#define WAV_CHANNEL_COUNT 1

#define WAV_MAGIC_RIFF 0x46464952
#define WAV_MAGIC_WAVE 0x45564157
#define WAV_MAGIC_FMT  0x20746D66
#define WAV_MAGIC_DATA 0x61746164

typedef struct
{
    u32 magicRiff;   // should be "RIFF"
    u32 sizeMinus8;  //
    u32 magicWave;   // should be "WAVE"
    u32 magicFmt;    // should be "fmt "
    u32 sectionSize; // should be 16
    u16 type;        // should be 0x01 (PCM)
    u16 channels;
    u32 sampleFreq;
    u32 dataRate; // bytes/second
    u16 align;
    u16 bitsPerSample;
    u32 magicData; // should be "data"
    u32 dataSize;
} wavHeader;

typedef struct
{
    wavHeader info;
    char*     path;
    u32       chunkSize;
    u32       chunkCount;
    u32       remainingBytes;
    FIL       fp;
} wavFile;

typedef enum
{
    WAV_OK,
    WAV_ERR_FILE_NOT_PRESENT,
    WAV_ERR_IO,
    WAV_ERR_READ_READ_MISMATCH,
    WAV_ERR_MAGIC,
    WAV_ERR_WRONG_CHANNEL_COUNT,
    WAV_ERR_WRONG_SAMPLE_SIZE,
    WAV_ERR_WRONG_SAMPLE_RATE,
    WAV_ERR_SEEK,
} wavStatus;

// the wav file needs to be as minimal as possible. achieved by running command 'ffmpeg -i "input.wav" -bitexact -ac 1 -map_metadata -1 "output.wav"
wavStatus wavLoadFile(wavFile* pOut, char* pPath, u32 pChunkSize);
wavStatus wavLoadChunk(wavFile* pFile, u32 chunkIndex, void* pOut, u32* pNumRead);
wavStatus wavUnloadFile(wavFile* pFile);

#endif //WAV_H
