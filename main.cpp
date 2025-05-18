#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <regex>
#include <vector>

#include "loop_detection.h"
#include "json/json.hpp"

extern "C" {
#include "sfs/synth_fs.h"
#include "types.h"
#include "solfege/solfege.h"
}

#define DESKTOP

#define opos(s) ((size_t)s.tellp())
#define ipos(s) ((size_t)s.tellg())
#define obpos(s) (opos(s) / BLOCK_SIZE)
#define ibpos(s) (ipos(s) / BLOCK_SIZE)

synthErrno     flashImage();
synthErrno     writeImage();
void           copyStream(std::ofstream& pOfstream, std::ifstream& pIfstream);
void           writeFileToOfstream(std::ofstream& pOfstream, const char* pFile);
void           padStream(std::ostream& pOstream, size_t pTo);
nlohmann::json loadJson(const char* pFile);

nlohmann::json loadJson(const char* pFile)
{
    std::ifstream file(pFile);
    auto          pJson = nlohmann::json::parse(file);
    file.close();

    return pJson;
}

void padStream(std::ostream& pOstream, size_t pTo)
{
    auto pos = opos(pOstream);
    if (pos % pTo == 0)
    {
        return;
    }

    for (int i = 0; i < pTo - (pos % pTo); ++i)
    {
        pOstream.put(0);
    }
}

void writeFileToOfstream(std::ofstream& pOfstream, const char* pFile)
{
    std::ifstream in(pFile, std::ios_base::in | std::ios_base::binary);
    copyStream(pOfstream, in);
    in.close();
}

void copyStream(std::ofstream& pOfstream, std::ifstream& pIfstream)
{
    while (!pIfstream.eof())
    {
        pOfstream.put(pIfstream.get());
    }
}

int main(int pArgc, char* pArgv[])
{
    const auto firstArg = std::string(pArgv[1]);

    synthErrno ret;
    if (firstArg == "-w")
    {
        ret = writeImage();
    }
    else if (firstArg == "-f")
    {
        ret = flashImage();
    }
    else
    {
        printf("Invalid argument.\n");
        ret = SERR_CMD_INVALID_ARGUMENT;
    }

    printf("Exiting with status %d\n", ret);
    return ret;
}

synthErrno writeImage()
{
    solfegeInit();

    std::ofstream sfsImgOut("synth.bin", std::ios_base::out | std::ios_base::binary);

    std::map<std::string, u16>       mapInstrumentStringIdToNumId;
    std::vector<sfsSingleInstrument> singleInstrumentPool;
    std::vector<sfsInstrumentSample> samplePool;
    std::vector<std::string>         namePool;
    std::vector<std::vector<u8>>     sampleDataPool;

    u32 currentSampleBlockOffset = 0;
    u32 currentNameOffset        = 0;
    u32 currentInstrumentId      = 0;

    u32 singleInstrumentCount = 0;
    u32 multiInstrumentCount  = 0;

    for (const auto& ent : std::filesystem::directory_iterator("instruments"))
    {
        if (!ent.is_directory())
        {
            continue;
        }

        const auto& instrumentPath    = ent.path();
        auto        instrumentDirName = instrumentPath.filename();
        auto        instrumentStrId   = instrumentDirName.string();

        std::ifstream  configJson(instrumentPath / "config.json");
        nlohmann::json config = nlohmann::json::parse(configJson);
        configJson.close();

        mapInstrumentStringIdToNumId[instrumentStrId] = currentInstrumentId;

        if (bool isSingle = config["single"])
        {
            singleInstrumentCount++;

            sfsSingleInstrument instrument = {};

            sfsSoundType soundType = SFS_SOUND_TYPE_ATTACK;

            std::string soundTypeStr = config["soundType"];
            if (soundTypeStr.contains("attack"))
            {
                soundType |= SFS_SOUND_TYPE_ATTACK;
            }
            if (soundTypeStr.contains("loop"))
            {
                soundType |= SFS_SOUND_TYPE_LOOP;
            }

            instrument.firstSampleBlock = currentSampleBlockOffset;
            instrument.nameStrIndex     = namePool.size();
            instrument.soundType        = soundType;
            instrument.fadeTimeForced   = config["fadeForced"];

            std::string name = config["name"];
            namePool.push_back(name);

            currentNameOffset += name.length() + 1;

            u8 noteRangeStart = 0xFF, noteRangeEnd = 0;

            // printf("Instrument %s\n", name.c_str());

            for (const auto& sampleFileEnt : std::filesystem::directory_iterator(ent))
            {
                auto filename = sampleFileEnt.path().filename();
                if (filename.extension().string() != ".wav")
                {
                    continue;
                }

                auto sampleName = filename.replace_extension().string();

                auto underscoreIdx     = sampleName.find('_');
                u8   sampleVelocity    = std::stoi(sampleName.substr(underscoreIdx + 1));
                u8   sampleSemitoneOff = std::stoi(sampleName.substr(0, underscoreIdx));

                if (sampleSemitoneOff > noteRangeEnd)
                {
                    noteRangeEnd = sampleSemitoneOff;
                }

                if (sampleSemitoneOff < noteRangeStart)
                {
                    noteRangeStart = sampleSemitoneOff;
                }

                std::ifstream fileStream(sampleFileEnt.path(), std::ios_base::binary | std::ios_base::in);
                std::string   readData;
                u32           sampleRate = 0;

                while (!fileStream.eof())
                {
                    readData += (char)fileStream.get();

                    std::string::size_type i;
                    if (!sampleRate && (i = readData.find("fmt ")) != std::string::npos)
                    {
                        fileStream.seekg(0xC - 4, std::ios_base::cur);
                        fileStream.read((str)&sampleRate, 4);
                        fileStream.seekg(-8 - 4, std::ios_base::cur);

                        if (sampleRate != SFS_SAMPLERATE)
                        {
                            return SERR_SFS_INVALID_SAMPLERATE;
                        }
                    }
                    else if ((i = readData.find("data")) != std::string::npos)
                    {
                        break;
                    }
                }

                u32 dataSize;
                fileStream.read((str)&dataSize, 4);

                u32 dataSizeAligned = roundUpTo(dataSize, BLOCK_SIZE);
                u32 dataSizeRem     = dataSize - dataSizeAligned;

                f32 sampleLengthSeconds    = dataSizeAligned / (2.0F * SFS_SAMPLERATE);
                instrument.fadeTimeDefault = sampleLengthSeconds;

                sfsInstrumentSample sample = {};

                sample.pcmDataLengthBlocks = dataSizeAligned / BLOCK_SIZE;
                sample.pcmDataBlockOffset  = currentSampleBlockOffset;

                currentSampleBlockOffset += sample.pcmDataLengthBlocks;

                if (soundType & SFS_SOUND_TYPE_LOOP)
                {
                    auto loopInfo = config["loopInfo"];
                    if (loopInfo.contains("preset") && !loopInfo["preset"].is_null())
                    {
                        std::string preset = loopInfo["preset"];
                        if (preset == "until end")
                        {
                            sample.loopStart    = 0;
                            sample.loopDuration = dataSizeAligned / sizeof(u16);
                        }
                        else if (preset == "until half")
                        {
                        }
                        else
                        {
                            return SERR_SFS_INVALID_LOOP_PRESET;
                        }
                    }
                    else
                    {
                        auto obj = loopInfo["notes"][std::to_string(sampleSemitoneOff)][std::to_string(sampleVelocity)];

                        sample.loopStart    = obj[0];
                        sample.loopDuration = obj[1];
                    }
                }
                else
                {
                    sample.loopStart    = 0;
                    sample.loopDuration = 0;
                }

                sample.velocity       = sampleVelocity;
                sample.pitchSemitones = sampleSemitoneOff;

                if (soundType & SFS_SOUND_TYPE_LOOP)
                {
                    // printf("\t- loop start:     %f (%05x)\n", sample.loopStart / SFS_SAMPLERATE_F32, sample.loopStart * 2);
                    // printf("\t- loop end:       %f (%05x)\n", (sample.loopStart + sample.loopDuration) / SFS_SAMPLERATE_F32, (sample.loopDuration + sample.loopStart) * 2);
                    // printf("\t- loop length:    %f (%05x)\n", sample.loopDuration / SFS_SAMPLERATE_F32, sample.loopDuration);
                }

                auto sampleData = std::vector<u8>(dataSizeAligned);
                fileStream.read((str)sampleData.data(), dataSize);

                if (dataSizeRem)
                {
                    for (size_t i = dataSize; i < dataSizeAligned; i++)
                    {
                        sampleData[i] = 0;
                    }
                }

                samplePool.push_back(sample);
                sampleDataPool.push_back(sampleData);

                fileStream.close();
            }

            instrument.noteRangeStart = noteRangeStart;
            instrument.noteRangeEnd   = noteRangeEnd;

            singleInstrumentPool.push_back(instrument);
        }
        else
        {
            multiInstrumentCount++;
        }

        currentInstrumentId++;
    }

    u32 instrumentCount = singleInstrumentCount + multiInstrumentCount;

    sfsHeader header = {};
    header.magic     = SFS_MAGIC;

    // font
    {
        sfsImgOut.seekp(BLOCK_SIZE * 1, std::ios_base::beg);
        header.fontDataBlockStart = obpos(sfsImgOut);
        writeFileToOfstream(sfsImgOut, "synth-font/font.bin");
        padStream(sfsImgOut, BLOCK_SIZE);
    }

    // hold behaviours
    {
        std::vector<std::vector<sfsHoldBehaviour>> holdBehaviours(singleInstrumentCount);

        sfsHoldBehaviour dummyHold = {};
        dummyHold.instrumentId     = SFS_INVALID_INSTRUMENT_ID;

        for (size_t i = 0; i < singleInstrumentCount; i++)
        {
            holdBehaviours[i] = std::vector(1, dummyHold);
        }

        size_t holdBehaviourCount = 0;

        nlohmann::json holdJson = loadJson("instruments/hold.json");
        for (auto it = holdJson.begin(); it != holdJson.end(); ++it)
        {
            auto key = it.key();

            auto behavioursJson = holdJson[key];
            if (behavioursJson.size() > holdBehaviourCount)
            {
                holdBehaviourCount = behavioursJson.size();
            }

            std::vector<sfsHoldBehaviour> behaviourVector;

            for (auto behaviour : behavioursJson)
            {
                sfsHoldBehaviour behaviourObj = {};

                behaviourObj.triggerTime    = behaviour["triggerTime"];
                behaviourObj.maxTriggerTime = behaviour["maxTriggerTime"];
                behaviourObj.transitionTime = behaviour["transitionTime"];
                behaviourObj.instrumentId   = mapInstrumentStringIdToNumId[behaviour["instrument"]];

                behaviourVector.push_back(behaviourObj);
            }

            auto keyRegex = std::regex(key);
            for (const auto& instrumentStrId : mapInstrumentStringIdToNumId | std::views::keys)
            {
                if (std::regex_match(instrumentStrId, keyRegex))
                {
                    auto instrumentNumId = mapInstrumentStringIdToNumId[instrumentStrId];
                    for (size_t i = 0; i < behaviourVector.size(); i++)
                    {
                        auto elem = behaviourVector[i];
                        if (elem.instrumentId == instrumentNumId)
                        {
                            behaviourVector.erase(behaviourVector.begin() + i);
                        }
                    }

                    holdBehaviours[instrumentNumId] = behaviourVector;
                }
            }
        }

        holdBehaviourCount++;

        for (size_t i = 0; i < singleInstrumentCount; i++)
        {
            auto& behaviours = holdBehaviours[i];
            behaviours.insert(behaviours.begin(), dummyHold);

            auto remaining = holdBehaviourCount - behaviours.size();
            for (size_t j = 0; j < remaining; ++j)
            {
                behaviours.push_back(dummyHold);
            }
        }

        header.holdBehaviorDataStart = obpos(sfsImgOut);

        for (size_t i = 0; i < singleInstrumentCount; i++)
        {
            auto behaviours = holdBehaviours[i];

            for (auto behaviour : behaviours)
            {
                auto ptr = (str)&behaviour;
                sfsImgOut.write(ptr, sizeof(sfsHoldBehaviour));
            }
        }

        padStream(sfsImgOut, BLOCK_SIZE);
    }

    // pcm data
    {
        auto pcmStart            = obpos(sfsImgOut);
        header.pcmDataBlockStart = pcmStart;

        for (auto& sample : samplePool)
        {
            sample.pcmDataBlockOffset += pcmStart;
        }

        for (auto pcm : sampleDataPool)
        {
            sfsImgOut.write((str)pcm.data(), pcm.size());
        }
    }

    // string LUT
    {
        auto lutStart              = obpos(sfsImgOut);
        header.stringLutBlockStart = lutStart;

        u32 offset = 0;
        for (const auto& name : namePool)
        {
            sfsImgOut.write((str)&offset, sizeof(u32));
            offset += name.length() + 1;
        }

        padStream(sfsImgOut, BLOCK_SIZE);
    }

    // string data
    {
        auto strDataStart           = obpos(sfsImgOut);
        header.stringDataBlockStart = strDataStart;

        for (const auto& name : namePool)
        {
            sfsImgOut.write(name.data(), name.length() + 1);
        }

        padStream(sfsImgOut, BLOCK_SIZE);
    }

    // sfsSingleInstrument data
    {
        auto instrumentInfoStart            = obpos(sfsImgOut);
        header.instrumentInfoDataBlockStart = instrumentInfoStart;

        for (const auto& instrument : singleInstrumentPool)
        {
            sfsImgOut.write((str)&instrument, sizeof(sfsSingleInstrument));
        }

        padStream(sfsImgOut, BLOCK_SIZE);
    }

    // sfsSingleSample data
    {
        auto sampleInfoStart        = obpos(sfsImgOut);
        header.sampleDataBlockStart = sampleInfoStart;

        for (const auto& sample : samplePool)
        {
            sfsImgOut.write((str)&sample, sizeof(sfsInstrumentSample));
        }

        padStream(sfsImgOut, BLOCK_SIZE);
    }

    header.instrumentCount       = instrumentCount;
    header.singleInstrumentCount = singleInstrumentCount;
    header.multiInstrumentCount  = multiInstrumentCount;

    sfsImgOut.seekp(0, std::ios_base::beg);
    sfsImgOut.write((str)&header, sizeof(header));

    sfsImgOut.close();

    return SERR_OK;
}

synthErrno flashImage()
{
    system("wmic diskdrive list brief");
    printf("Select physical drive: ");

    char pDriveBuf[32];
    gets(pDriveBuf);

    char drive = pDriveBuf[0];

    printf("Flashing drive %c. Confirm (yes): ", drive);

    char input[32];
    gets(input);

    if (input != std::string("yes"))
    {
        printf("Aborting.\n");
        return SERR_OK;
    }

    printf("Flashing...\n");

    char cmd[256];
    sprintf(cmd, "physdiskwrite -u -d %c synth.bin", drive);
    printf("Running command: '%s'\n", cmd);
    return (synthErrno)system(cmd);
}
