#include <iostream>
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <cstdio>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <cmath>
#include <chrono>
#include <thread>
using namespace std;

#define _DEBUG
#ifndef AL_CHECK
#ifdef _DEBUG
    #define ALCHECK(func) do{ \
        func; \
        CheckOpenALError(#func, __FILE__, __LINE__); \
    }while(0);
#else
     #define ALCHECK(func) func
#endif
#endif


#define LoadMaxBit    (44100 * 2 * 16)

const char * GetOpenALErrorString(int errID)
{   
    if (errID == AL_NO_ERROR) return "";
    if (errID == AL_INVALID_NAME) return "Invalid name";
    if (errID == AL_INVALID_ENUM) return " Invalid enum ";
    if (errID == AL_INVALID_VALUE) return " Invalid value ";
    if (errID == AL_INVALID_OPERATION) return " Invalid operation ";
    if (errID == AL_OUT_OF_MEMORY) return " Out of memory like! ";
    return " Don't know ";  
}

inline void CheckOpenALError(const char* func, const char* filename, int line)
{
    ALenum err = alGetError();
    if(err != AL_NO_ERROR)
    {
        printf("AL ERROR: %08x, (%s) at %s:%i - for %s\n", err,  GetOpenALErrorString(err), filename, line, func);
    }
}

size_t fileSize(FILE* f)
{
    fseek(f, 0, SEEK_END);
    size_t filesize = ftell(f);
    fseek(f, 0, SEEK_SET);
    return filesize;
}


class AL
{
public:
    AL()
    {
        // setup OpenAL context and make it current
        this->alcDevice = alcOpenDevice(NULL);
        if (nullptr == this->alcDevice)
        {
            printf("alcOpenDevice() failed!\n");
        }
        this->alcContext = alcCreateContext(this->alcDevice, NULL);
        if (nullptr == this->alcContext)
        {
            printf("alcCreateContext() failed!\n");
        }
        if (!alcMakeContextCurrent(this->alcContext))
        {
            printf("alcMakeContextCurrent() failed!\n");
        }
    }
    void PrintInfo()
    {
        const ALCchar* alcStr = alcGetString(this->alcDevice, ALC_DEVICE_SPECIFIER);
        if (alcStr)
        {
            printf("ALC_DEVICE_SPECIFIER: %s\n", alcStr);
        }
        alcStr = alcGetString(this->alcDevice, ALC_EXTENSIONS);
        if (alcStr)
        {
            printf("ALC_EXTENSIONS:\n%s\n", alcStr);
        }
        const ALchar* alStr = alGetString(AL_VERSION);
        if (alStr)
        {
            printf("AL_VERSION: %s\n", alStr);
        }
        alStr = alGetString(AL_RENDERER);
        if (alStr)
        {
            printf("AL_RENDERER: %s\n", alStr);
        }
        alStr = alGetString(AL_VENDOR);
        if (alStr)
        {
            printf("AL_VENDOR: %s\n", alStr);
        }
        alStr = alGetString(AL_EXTENSIONS);
        if (alStr)
        {
            printf("AL_EXTENSIONS:\n%s\n", alStr);
        }
    }

    ~AL()
    {
        alcMakeContextCurrent(NULL);
        alcDestroyContext(alcContext);
        alcCloseDevice(alcDevice);
    }
private:
    ALCdevice* alcDevice;
    ALCcontext* alcContext;

};

class ALSource
{
public:
    ALSource()
    {
        ALCHECK(alGenSources(1, &sid));
        ALCHECK(alSourcef(sid, AL_GAIN, 1));
        ALCHECK(alSourcef(sid, AL_PITCH, 1));
        ALCHECK(alSourcei(sid, AL_LOOPING, AL_FALSE));
        ALCHECK(alSource3f(sid, AL_POSITION, 0.0f, 0.0f, 0.0f));
        ALCHECK(alSource3f(sid, AL_VELOCITY, 0.0f, 0.0f, 0.0f));
    }

    void SetVolume(float volume)
    {
        ALCHECK(alSourcef(sid, AL_GAIN, volume));
    }
    void SetPitch(int pitch)
    {
        ALCHECK(alSourcef(sid, AL_PITCH, pitch));

    }
    void SetLooping(bool loop)
    {
        ALCHECK(alSourcei(sid, AL_LOOPING, loop ? AL_TRUE : AL_FALSE));
    }

    void SetPosition(float x, float y, float z)
    {
        ALCHECK(alSource3f(sid, AL_POSITION, x, y, z));
    }
    void SetVelocity(float x, float y, float z)
    {
        ALCHECK(alSource3f(sid, AL_VELOCITY, x, y, z));
    }

    ALuint GetBufferID()
    {
        ALint bufferid;
        alGetSourcei(sid, AL_BUFFER, &bufferid);
        return bufferid;
    }

    ALint GetSourceState()
    {
        ALint sourceState;
        alGetSourcei(sid, AL_SOURCE_STATE, &sourceState);
        return sourceState;
    } 

    bool IsPaused()
    {
        return (AL_PAUSED == GetSourceState());
    }
    bool IsStopped()
    {
        return (AL_STOPPED == GetSourceState());
    }
 
    bool IsPlaying()
    {
        return (AL_PLAYING == GetSourceState());
    }
    bool IsAtBeginning()
    {
        return (AL_INITIAL  == GetSourceState());
    }
    void SetBuffer(ALuint bufferid)
    {
        ALCHECK(alSourcei(sid, AL_BUFFER, bufferid));
    }
    void SetBuffers(int n, ALuint& bid)
    {
        ALCHECK(alSourceQueueBuffers(sid, n, &bid));
    }
    void Play()
    {
        ALCHECK(alSourcePlay(sid));
    }

    void Stop()
    {
        alSourceStop(sid);
    }
    void Pause()
    {
        alSourcePause(sid);
    }


    float GetProgress()
    {
        ALfloat p;
        alGetSourcef(sid, AL_BYTE_OFFSET, &p);
        return p;
    }

    int GetBufferCounts()
    {
        ALint bf;
        alGetSourcei(sid, AL_BUFFERS_QUEUED, &bf);
        return bf;
    }
    int GetBufferProcessedCounts()
    {
        ALint bf;
        alGetSourcei(sid, AL_BUFFERS_PROCESSED, &bf);
        return bf;
    }
    void UnQueueBuffers(int n)
    {
        ALuint bid;
        alSourceUnqueueBuffers(sid, n, &bid);
    }
    ~ALSource()
    {
        alDeleteSources(1, &sid);
    }

    int indicator;
    ALuint sid;
};

class ALBuffer
{
public:
    ALBuffer()
    {
        ALCHECK(alGenBuffers(1, &bid));
    }
    ALuint loadSound(ALuint audioType, char* data, int size, int samplerate)
    {
        ALCHECK(alBufferData(bid, audioType, data, size, samplerate));
        return 0;
    }
    ~ALBuffer()
    {
        ALCHECK(alDeleteBuffers(1, &bid));
    }
    ALuint bid;
};


class ALListener
{
public:
    ALListener()
    {
        alListener3f(AL_POSITION, 0.0f, 0.0f, 0.0f);
        alListener3f(AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    }
    void SetPosition(float x, float y, float z)
    {
        alListener3f(AL_POSITION, x, y, z);
    }
    void SetVelocity(float x, float y, float z)
    {
        alListener3f(AL_VELOCITY, x, y, z);
    }
};

class WavFile
{
public:
    WavFile(){}
    explicit WavFile(const char* filename)
    {
        Setup(filename);
    }
    void Setup(const char* filename)
    {
        f = fopen(filename, "r");
        assert(f);
        fread(ChunkID, sizeof(char), 4, f);
        fread(&ChunkSize, sizeof(int32_t), 1, f);
        fread(format, sizeof(char), 4, f);
        fread(SubChunk1ID, sizeof(char), 4, f);
        fread(&SubChunk1Size, sizeof(int32_t), 1, f);
        fread(&AudioFormat, sizeof(int16_t), 1, f);
        fread(&NumChannels, sizeof(int16_t), 1, f);
        fread(&SampleRate, sizeof(int32_t), 1, f);
        fread(&ByteRate, sizeof(int32_t), 1, f);
        fread(&BlockAlign, sizeof(int16_t), 1, f);
        fread(&BitsPerSample, sizeof(int16_t), 1, f);
        fread(&SubChunk2ID, sizeof(char), 4, f);
        fread(&SubChunk2Size, sizeof(int32_t), 1, f);


        if(ByteRate >= SubChunk2Size) //less than 1 second
        {
            isNoMoreData = true;
            bufferSize = SubChunk2Size;
        }
        else
        {
            isNoMoreData = false;
            bufferSize = ByteRate;
        }
        data = (char*)malloc(bufferSize * sizeof(char));
        fread(data, sizeof(char), bufferSize, f);
        // data = (char*)malloc(SubChunk2Size * sizeof(char));
        // fread(data, sizeof(char), SubChunk2Size, f);

        duration = (float)SubChunk2Size / (float)ByteRate;
        cursor = bufferSize;
        ChunkID[4] = '\0';
        format[4] = '\0';
        SubChunk1ID[4] = '\0';
        SubChunk2ID[4] = '\0';
    }
    bool ReadMore()
    {
        if(cursor >= SubChunk2Size) return false;
        int leftDataSize = SubChunk2Size - cursor;
        if(leftDataSize > bufferSize)
        {
            fread(data, sizeof(char), bufferSize, f);
            cursor += bufferSize;
            assert(cursor < SubChunk2Size);
        }
        else
        {
            //When leftData's size is less than ByteRate, we have to clear the memory
            //Otherwise we may hear some starnge sound after the true buffer content is played.
            
            //NOTE: the buffer size is not always equal to ByteRate because when the 
            //wav file's duration is less than 1 seconds, we will allocate the buffer with
            //data size instead of ByteRate. However we we allocate data size buffer,
            //the cursor is equal to SubChunk2Size which will never happen here.
            
            assert(ByteRate <  SubChunk2Size);
            std::memset(data, 0x00, bufferSize);
            fread(data, sizeof(char), leftDataSize, f);
            isNoMoreData = true;
            cursor += leftDataSize;
        }
        // printf("Total cursor: %d\n", cursor);
        return true;
    }
    void SetPos(int Pos)
    {
        cursor = Pos;
    }
    ~WavFile()
    {
        if(this->data)
        {
            free(this->data);
            this->data = nullptr;
        }
        fclose(f);
    }
    void PrintInfo()
    { 
        printf("ChunkID = %s \nChunkSize = %d \nformat = %s \nSubChunk1ID = %s \nSubChunk1Size = %d \nAudioFormat = %d \nNumChannels = %d \nSampleRate = %d \nByteRate = %d \nBlockAlign = %d \nBitsPerSample = %d \nSubChunk2ID = %s \nSubChunk2Size = %d \nduration = %.2f \n",
            ChunkID ,ChunkSize ,format ,SubChunk1ID ,SubChunk1Size ,AudioFormat ,NumChannels ,SampleRate ,ByteRate ,BlockAlign ,BitsPerSample ,SubChunk2ID ,SubChunk2Size, duration
            );
    }

    // Chunk
    char ChunkID[5]; // 0 terminate
    int32_t ChunkSize;
    char format[5];
    
    // Fmt Chunk
    char SubChunk1ID[5];
    int16_t SubChunk1Size;
    int16_t AudioFormat;
    int16_t NumChannels;
    int32_t SampleRate;
    int32_t ByteRate;
    int16_t BlockAlign;
    int16_t BitsPerSample;
    
    //Data Chunk
    char SubChunk2ID[5];
    int32_t SubChunk2Size;
    char* data;
    float duration;

    FILE* f;
    int cursor;
    bool isNoMoreData;
    int bufferSize;
};



class MusicPlayer
{
public:
    MusicPlayer(){}
    MusicPlayer(const MusicPlayer&) = delete;
    MusicPlayer(const char* file)
    {
        Setup(file);
    }
    void Setup(const char* filename)
    {
        playCursor = 0;
        bufferCounts = 4;
        wavf.Setup(filename);
        albv.resize(bufferCounts);

        int cursor = 0;
        int oneTime = ceil(wavf.bufferSize / bufferCounts);
        for(ALBuffer& b : albv)
        {
            int left = (wavf.bufferSize - cursor);
            int size = min(left, oneTime);
            b.loadSound(AL_FORMAT_STEREO16, &wavf.data[cursor], size, wavf.SampleRate);
            als.SetBuffers(1, b.bid);
            cursor += size;
        }

        playCursor += wavf.bufferSize;

        current = 0;
        isNeedMoreData = true;
        isEnd = false;
    }
    bool IsPlaying()
    {
        return als.IsPlaying();
    }
    void Play()
    {
        als.Play();
    }
    void Pause()
    {
        als.Pause();
    }

    void FillBuffer()
    {
        if(isEnd) return;
        int fillcount = als.GetBufferProcessedCounts();
        while(fillcount--)
        {
            // assert(fillcount <= 1);
            als.UnQueueBuffers(1);
            if(isNeedMoreData)
            {
                printf("MoreData\n");
                if(!wavf.ReadMore())
                {
                    isEnd = true;
                    return;
                }
                isNeedMoreData = false;
            }
            int oneTime = ceil(wavf.bufferSize / bufferCounts);
            current = min((wavf.bufferSize - current), oneTime);
            //Fill Data
            
            albv[currentBuffer].loadSound(AL_FORMAT_STEREO16, &wavf.data[currentBuffer * oneTime], current, wavf.SampleRate);
            als.SetBuffers(1, albv[currentBuffer].bid);
            playCursor += current;
            // printf("oneTime: %d, current:%d, playCursor:%d\n", oneTime, current, playCursor);
            if(++currentBuffer >= albv.size())
            {
                currentBuffer = 0;
                current = 0;
                isNeedMoreData = true; // moredata
            }
        }
        printf("current progress: %2.2f/%2.2f\n", GetProgress(), GetDuration());

        // printf("slc current buffer id: %d\n", als.GetBufferID());
        // printf("slc buffers counts: %d\n", als.GetBufferCounts());
        // printf("slc processed buffers counts: %d\n", als.GetBufferProcessedCounts());
    }
    float GetProgress()
    {
        return (float)playCursor / (float)wavf.ByteRate;
    }
    float GetDuration()
    {
        return wavf.duration;
    }

    int32_t playCursor; // progress
    vector<ALBuffer> albv;
    int currentBuffer;
    ALSource als;
    WavFile wavf;
    int current;

    bool isNeedMoreData;
    int bufferCounts;
    bool isEnd;
};

// implement in use load functionality
int main(int argc, char const *argv[])
{
    WavFile wavf2("bounce.wav");
    AL al;
    ALBuffer alb;
    // ALBuffer albv;
    alb.loadSound(AL_FORMAT_MONO16, wavf2.data, wavf2.SubChunk2Size, wavf2.SampleRate);
    ALSource als2;
    ALListener alL;


    als2.SetBuffer(alb.bid);
    als2.SetLooping(true);

    MusicPlayer als("3.wav");
    als.Play();
    while(1)
    {
        this_thread::sleep_for(chrono::milliseconds(100));
        als.FillBuffer();
        if(als.isEnd) break;
    }

    char c;

    // chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();
    // while(scanf("%c", &c) && c != 'q')
    // {
    //     if(c == 'p')
    //     {
    //         if(als.IsPlaying())
    //         {
    //             printf("Audio pause.\n");
    //             als.Pause();
    //         }
    //         else
    //         {
    //             printf("audio play.\n");
    //             als.Play();
    //         }
    //     }
    //     else if(c == 'b')
    //     {
    //         if(als2.IsPlaying())
    //         {
    //             als2.Stop();
    //         }
    //         else
    //         {
    //             als2.Play();
    //         }
    //     }
    //     else
    //     {
    //         als.FillBuffer();
    //         printf("current progress: %f/%2.2f", als.GetProgress() / als.GetDuration(), als.GetDuration());
    //     }
    // }
    return 0;
}