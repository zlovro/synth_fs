//
// Created by Made on 09/05/2025.
//

#include "loop_detection.h"

synthErrno loopDetect(std::istream& pPcmStream, u16 pSampleRate, f32 pLoopStart, f32 pLoopMinDuration, loopDetectionOutInfo* pOutInfo)
{
    u32 pcmDataStart = pPcmStream.tellg();

    u32 endSample   = 0;
    u32 startSample = 0;

    u32 loopStartSample    = (u32)(pLoopStart * pSampleRate);
    u32 loopEndStartSample = loopStartSample + (pLoopMinDuration * pSampleRate);
    u32 loopEndEndSample   = loopEndStartSample + 0xFFFF;

    u32 windowLength = 1.0e-3F * LOOP_DETECTION_WINDOW_SIZE_MS * pSampleRate;

    s16 mx            = INT16_MIN;
    s16 amplitude     = INT16_MIN;
    s16 nextAmplitude = INT16_MIN;

    pPcmStream.seekg(pcmDataStart + loopStartSample * 2, std::ios_base::beg);

    for (u32 i = loopStartSample; i < loopStartSample + windowLength; i++)
    {
        pPcmStream.read((str)&amplitude, 2);
        pPcmStream.read((str)&nextAmplitude, 2);

        if (amplitude > nextAmplitude && amplitude > mx)
        {
            // printf("\t\tnew max: %05d (%05.5f, %05x)\n", amplitude, i / 48000.0, i * 2);
            mx          = amplitude;
            startSample = i;
        }

        pPcmStream.seekg(-2, std::ios_base::cur);
    }

    pPcmStream.seekg(pcmDataStart + loopEndStartSample * 2, std::ios_base::beg);

    s16 mx2           = INT16_MIN;
    s16 closest       = INT16_MIN;
    s16 lastAmplitude = INT16_MIN;
    for (u32 i = loopEndStartSample; i < loopEndEndSample - 1; i++)
    {
        pPcmStream.read((str)&amplitude, 2);
        pPcmStream.read((str)&nextAmplitude, 2);

        // if (amplitude > nextAmplitude && amplitude > mx2)
        // {
        //     mx2       = lastAmplitude;
        //     endSample = i - 1;
        // }

        if (amplitude > nextAmplitude && amplitude > 0 && abs((s32)mx - amplitude) < abs((s32)mx - closest))
        {
            printf("\t\t%05d:%05d --- %05d < %05d --> closest = %05d\n", mx, amplitude, abs((s32)mx - amplitude), abs((s32)mx - closest), amplitude);
            closest = amplitude;
            endSample = i;
        }


        lastAmplitude = amplitude;

        pPcmStream.seekg(-2, std::ios_base::cur);
    }

    pOutInfo->averageAmplitudeFalloff = mx - closest;
    // pOutInfo->averageAmplitudeFalloff = mx - mx2;
    pOutInfo->loopStart = startSample;
    pOutInfo->loopEnd   = endSample;

    pPcmStream.seekg(pcmDataStart, std::ios_base::beg);

    return SERR_OK;
}
