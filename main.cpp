#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <algorithm>

#include "sfs/synth_fs.h"

extern "C" {
#include "types.h"
#include "solfege/solfege.h"
}

#define DESKTOP

int main(void)
{
    solfegeInit();

    for (const auto& ent : std::filesystem::directory_iterator("instruments"))
    {
        if (!ent.is_directory())
        {
            continue;
        }

        const auto& instrumentPath = ent.path();
        auto        instrumentName = instrumentPath.filename();

        auto settingsFileStream = std::ifstream(instrumentPath / "settings.properties");

        bool isSingle = true;
        sfsSingleInstrument singleInstrument;
        sfsMultiInstrument multiInstrument;

        std::string line;
        while (std::getline(settingsFileStream, line))
        {
            auto delimIdx = line.find('=');
            if (delimIdx == std::string::npos)
            {
                continue;
            }

            std::string key = line.substr(0, delimIdx);
            std::string value = line.substr(delimIdx + 1);

            if (key == "single")
            {
                isSingle = value == "true";
            }

            if (isSingle)
            {

            }
        }

        for (const auto& sampleFileEnt : std::filesystem::directory_iterator(ent))
        {
            auto filename = sampleFileEnt.path().filename();
            if (filename.extension().string() != ".wav")
            {
                continue;
            }

            auto sampleName = filename.replace_extension().string();

            tone       sampleTone;
            u8         sampleVelocity;
            synthErrno ret;

            auto underscoreIdx = sampleName.find('_');

            std::string noteString;
            if (underscoreIdx == std::string::npos)
            {
                sampleVelocity = 0xFF;
                ret            = solfegeParseNote((str)sampleName.c_str(), &sampleTone);
            }
            else
            {
                sampleVelocity = std::stoi(sampleName.substr(underscoreIdx + 1));
                ret            = solfegeParseNote((str)sampleName.substr(0, underscoreIdx).c_str(), &sampleTone);
            }

            if (ret != SERR_OK)
            {
                synthPanic(ret);
            }

            std::ifstream fileStream(sampleFileEnt.path(), std::ios_base::binary | std::ios_base::in);

            char toSearch[]    = {'d', 'a', 't', 'a'};
            int  searchCounter = 0;

            u32 dataSize;
            while (!fileStream.eof())
            {
                if (fileStream.get() == toSearch[searchCounter])
                {
                    if (searchCounter == 3)
                    {
                        fileStream.read((str)&dataSize, 4);
                        break;
                    }
                    searchCounter++;
                }
            }

            u32 dataSizeAligned = roundUpTo(dataSize, BLOCK_SIZE);
            u32 dataSizeRem     = dataSize - dataSizeAligned;

            auto buf = new u8[dataSizeAligned];
            fileStream.read((str)buf, dataSize);

            if (dataSizeRem)
            {
                for (u32 i = dataSize; i < dataSizeAligned; ++i)
                {
                    buf[i] = 0;
                }
            }

            sfsInstrumentSample sample;

            sample.pcmDataLengthBlocks = dataSizeAligned / BLOCK_SIZE;
            sample.velocity            = sampleVelocity;
            sample.pitchSemitones      = sampleTone.semitoneOffset;

            delete[] buf;
        }
    }

    return 0;
}
