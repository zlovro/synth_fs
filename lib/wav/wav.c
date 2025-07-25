//
// Created by Made on 26/01/2025.
//

#include "wav.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

u32 wavWrite(u8 *pDst, u8 pBitsPerSample, u8 pChannels, u32 pSampleRate, u8 *pPcmData, u32 pPcmDataLength) {
    u32 wavSize = sizeof(wavHeader) + pPcmDataLength;

    wavHeader wavHdr     = {};
    wavHdr.magicRiff     = WAV_MAGIC_RIFF;
    wavHdr.sizeMinus8    = wavSize - 8;
    wavHdr.magicWave     = WAV_MAGIC_WAVE;
    wavHdr.magicFmt      = WAV_MAGIC_FMT;
    wavHdr.sectionSize   = 16;
    wavHdr.type          = 1;
    wavHdr.channels      = pChannels;
    wavHdr.sampleFreq    = pSampleRate;
    wavHdr.bitsPerSample = pBitsPerSample;
    wavHdr.dataRate      = (wavHdr.channels * wavHdr.sampleFreq * wavHdr.bitsPerSample) / 8;
    wavHdr.align         = (wavHdr.channels * wavHdr.bitsPerSample) / 8;
    wavHdr.magicData     = WAV_MAGIC_DATA;
    wavHdr.dataSize      = pPcmDataLength;

    memcpy(pDst, (u8 *) &wavHdr, sizeof(wavHeader));
    memcpy(pDst + sizeof(wavHeader), pPcmData, pPcmDataLength);

    return wavSize;
}

#ifdef DESKTOP
void wavWriteFile(const char *pFilePath, u8 pBitsPerSample, u8 pChannels, u32 pSampleRate, u8 *pPcmData, u32 pPcmDataLength) {
    u32 size = sizeof(wavHeader) + pPcmDataLength;
    u8* dat = malloc(size);
    wavWrite(dat, pBitsPerSample, pChannels, pSampleRate, pPcmData, pPcmDataLength);

    FILE *fp = fopen(pFilePath, "wb");
    fwrite(dat, 1, size, fp);
    free(dat);
    fclose(fp);
}
#endif
