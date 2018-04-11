#include <iostream>
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <cstdio>
#include <cassert>
#include <cstdint>
#include <cstdlib>


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

    bool IsStopped()
    {
        ALint sourceState;
        alGetSourcei(sid, AL_SOURCE_STATE, &sourceState);
        return (AL_STOPPED == sourceState);
    }
    void Play(ALuint bufferid)
    {
        ALCHECK(alSourcei(sid, AL_BUFFER, bufferid));
        ALCHECK(alSourcePlay(sid));
    }

    void Stop()
    {
        alSourceStop(sid);
    }

    bool IsPlaying()
    {
        ALint sourceState;
        alGetSourcei(sid, AL_SOURCE_STATE, &sourceState);
        return (AL_PLAYING == sourceState);
    }
    float GetProgress()
    {
        ALfloat p;
        alGetSourcef(sid, AL_BYTE_OFFSET, &p);
        return p;
    }

    ~ALSource()
    {
        alDeleteSources(1, &sid);
    }
    ALuint sid;
};

class ALBuffer
{
public:
    ALBuffer()
    {
        ALCHECK(alGenBuffers(1, &bid));
    }
    ALuint loadSound(ALuint audioType, void* data, int size, int samplerate)
    {
        alBufferData(bid, audioType, data, size, samplerate);
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
    explicit WavFile(FILE* f)
    {
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

        data = (char*)malloc(sizeof(char) * SubChunk2Size);
        fread(data, sizeof(char), SubChunk2Size, f);
        duration = (float)SubChunk2Size / (float)ByteRate;
        ChunkID[4] = '\0';
        format[4] = '\0';
        SubChunk1ID[4] = '\0';
        SubChunk2ID[4] = '\0';
    }

    ~WavFile()
    {
        if(this->data)
        {
            free(this->data);
            this->data = nullptr;
        }
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
};


class AudioPlayer
{
public:
    
int32_t playCursor;
};

// implement in use load functionality
int main(int argc, char const *argv[])
{
    FILE* f = fopen("2_.wav", "r");
    FILE* f2 = fopen("bounce.wav", "r");
    WavFile wavf(f);
    WavFile wavf2(f2);
    fclose(f);
    fclose(f2);

    AL al;
    al.PrintInfo();
    ALBuffer alb, alb2;
    alb.loadSound(AL_FORMAT_STEREO16, wavf.data, wavf.SubChunk2Size, wavf.SampleRate);
    alb2.loadSound(AL_FORMAT_MONO16, wavf2.data, wavf2.SubChunk2Size, wavf2.SampleRate);
    ALSource als, als2;
    ALListener alL;

    char c;
    float volume = 0.1;
    als.SetVolume(volume);
    als2.SetLooping(true);
    while(scanf("%c", &c) && c != 'q')
    {

        if(c == 'p' && !als.IsPlaying())
        {
            printf("audio play.\n");
            als.Play(alb.bid);
        }
        else if(c == 'b')
        {
            if(als2.IsPlaying())
            {
                als2.Stop();
            }
            else
            {
                als2.Play(alb2.bid);
            }
        }
        else if(c == 'u')
        {
            volume += 0.1f;
            als.SetVolume(volume);
        }
        else
        {
            printf("current progress: %f/%2.2f", als.GetProgress() / wavf.ByteRate, wavf.duration);
        }

    }


    return 0;
}