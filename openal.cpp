#include <iostream>
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <cstdio>
#include <cassert>
#include <cstdint>
#include <cstdlib>

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

ALCdevice* alcDevice;
ALCcontext* alcContext;
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
        printf("AL ERROR: %08x, (%s) at %s:%i - for %s", err,  GetOpenALErrorString(err), filename, line, func);
    }
}

void printALInfo()
{
    const ALCchar* alcStr = alcGetString(alcDevice, ALC_DEVICE_SPECIFIER);
    if (alcStr)
    {
        printf("ALC_DEVICE_SPECIFIER: %s\n", alcStr);
    }
    alcStr = alcGetString(alcDevice, ALC_EXTENSIONS);
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

bool AlInit(bool isPrintInfo)
{
    // setup OpenAL context and make it current
    alcDevice = alcOpenDevice(NULL);
    if (nullptr == alcDevice)
    {
        printf("alcOpenDevice() failed!\n");
        return false;
    }
    alcContext = alcCreateContext(alcDevice, NULL);
    if (nullptr == alcContext)
    {
        printf("alcCreateContext() failed!\n");
        return false;
    }
    if (!alcMakeContextCurrent(alcContext))
    {
        printf("alcMakeContextCurrent() failed!\n");
        return false;
    }
    
    if (isPrintInfo) printALInfo();

    return true;
}

size_t fileSize(FILE* f)
{
    fseek(f, 0, SEEK_END);
    size_t filesize = ftell(f);
    fseek(f, 0, SEEK_SET);
    return filesize;
} 


struct WavFile
{
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

    int32_t duration;
};



WavFile wavf;


void printWavInfo(WavFile& wavf)
{ 
    printf("wavf.ChunkID = %s \nwavf.ChunkSize = %d \nwavf.format = %s \nwavf.SubChunk1ID = %s \nwavf.SubChunk1Size = %d \nwavf.AudioFormat = %d \nwavf.NumChannels = %d \nwavf.SampleRate = %d \nwavf.ByteRate = %d \nwavf.BlockAlign = %d \nwavf.BitsPerSample = %d \nwavf.SubChunk2ID = %s \nwavf.SubChunk2Size = %d \n"
        ,wavf.ChunkID 
        ,wavf.ChunkSize 
        ,wavf.format 
        ,wavf.SubChunk1ID 
        ,wavf.SubChunk1Size 
        ,wavf.AudioFormat 
        ,wavf.NumChannels 
        ,wavf.SampleRate 
        ,wavf.ByteRate 
        ,wavf.BlockAlign 
        ,wavf.BitsPerSample 
        ,wavf.SubChunk2ID 
        ,wavf.SubChunk2Size
        );
}

void processWav(FILE*f)
{
    fread(wavf.ChunkID, sizeof(char), 4, f);
    fread(&wavf.ChunkSize, sizeof(int32_t), 1, f);
    fread(wavf.format, sizeof(char), 4, f);
    fread(wavf.SubChunk1ID, sizeof(char), 4, f);
    fread(&wavf.SubChunk1Size, sizeof(int32_t), 1, f);
    fread(&wavf.AudioFormat, sizeof(int16_t), 1, f);
    fread(&wavf.NumChannels, sizeof(int16_t), 1, f);
    fread(&wavf.SampleRate, sizeof(int32_t), 1, f);
    fread(&wavf.ByteRate, sizeof(int32_t), 1, f);
    fread(&wavf.BlockAlign, sizeof(int16_t), 1, f);
    fread(&wavf.BitsPerSample, sizeof(int16_t), 1, f);
    fread(&wavf.SubChunk2ID, sizeof(char), 4, f);
    fread(&wavf.SubChunk2Size, sizeof(int32_t), 1, f);

    wavf.data = (char*)malloc(sizeof(char) * wavf.SubChunk2Size);
    fread(wavf.data, sizeof(char), wavf.SubChunk2Size, f);

}

// implement in use load functionality

int main(int argc, char const *argv[])
{
    FILE* f = fopen("bounce.wav", "rw");
    assert(f);
    processWav(f);
    printWavInfo(wavf);

    free(wavf.data);
    wavf.data = nullptr;
    fclose(f);
    return 0;
}