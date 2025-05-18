//
// Created by Made on 09/05/2025.
//

#ifndef LOOP_DETECTION_H
#define LOOP_DETECTION_H
#include <fstream>
#include <synth_errno.h>

#endif //LOOP_DETECTION_H

#define LOOP_DETECTION_WINDOW_SIZE_MS 500

typedef struct
{
    u32 loopStart, loopEnd;
    s16 averageAmplitudeFalloff;
} loopDetectionOutInfo;

// position of pPcmStream must be start of pcmData
synthErrno loopDetect(std::istream& pPcmStream, u16 pSampleRate, f32 pLoopStart, f32 pLoopMinDuration, loopDetectionOutInfo* pOutInfo);