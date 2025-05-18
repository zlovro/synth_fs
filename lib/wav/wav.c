//
// Created by Made on 26/01/2025.
//

#include "wav.h"

#include <string.h>
#include <fatfs/ff.h>

wavStatus wavLoadFile(wavFile* pOut, char* pPath, u32 pChunkSize)
{
    FIL     fp;
    FRESULT res = f_open(&fp, pPath, FA_READ);
    if (res != FR_OK)
    {
        return WAV_ERR_FILE_NOT_PRESENT;
    }

    pOut->path      = pPath;
    pOut->chunkSize = pChunkSize;

    memcpy(&pOut->fp, &fp, sizeof(FIL));

    UINT bytesRead;
    res = f_read(&fp, &pOut->info, sizeof(wavHeader), &bytesRead);
    if (res != FR_OK)
    {
        return WAV_ERR_IO;
    }

    if (bytesRead != sizeof(wavHeader))
    {
        return WAV_ERR_READ_READ_MISMATCH;
    }

    if (pOut->info.magicData != WAV_MAGIC_DATA || pOut->info.magicFmt != WAV_MAGIC_FMT || pOut->info.magicRiff != WAV_MAGIC_RIFF || pOut->info.magicWave != WAV_MAGIC_WAVE)
    {
        return WAV_ERR_MAGIC;
    }

    if (pOut->info.channels != WAV_CHANNEL_COUNT)
    {
        return WAV_ERR_WRONG_CHANNEL_COUNT;
    }

    if (pOut->info.bitsPerSample != WAV_SAMPLE_SIZE_BITS)
    {
        return WAV_ERR_WRONG_SAMPLE_SIZE;
    }

    if (pOut->info.sampleFreq != WAV_SAMPLE_RATE)
    {
        return WAV_ERR_WRONG_SAMPLE_RATE;
    }

    u32 dataSize = f_size(&fp);

    pOut->chunkCount     = dataSize / pChunkSize;
    pOut->remainingBytes = dataSize % pChunkSize;

    return WAV_OK;
}

wavStatus wavLoadChunk(wavFile* pFile, u32 chunkIndex, void* pOut, u32* pNumRead)
{
    FRESULT res = f_lseek(&pFile->fp, chunkIndex * pFile->chunkSize);
    if (res != FR_OK)
    {
        return WAV_ERR_SEEK;
    }

    UINT readBytes;
    u32  toRead = chunkIndex == pFile->chunkCount - 1 ? pFile->remainingBytes : pFile->chunkSize;
    res = f_read(&pFile->fp, pOut, toRead, &readBytes);

    if (res != FR_OK)
    {
        return WAV_ERR_IO;
    }

    if (toRead != readBytes)
    {
        return WAV_ERR_READ_READ_MISMATCH;
    }

    *pNumRead = readBytes;

    return WAV_OK;
}

wavStatus wavUnloadFile(wavFile* pFile)
{
    return f_close(&pFile->fp) == FR_OK ? WAV_OK : WAV_ERR_IO;
}
