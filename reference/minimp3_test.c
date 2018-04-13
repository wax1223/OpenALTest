/*#define MINIMP3_ONLY_MP3*/
/*#define MINIMP3_ONLY_SIMD*/
/*#define MINIMP3_NONSTANDARD_BUT_LOGICAL*/
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#if defined(_MSC_VER)
    #define strcasecmp(str1, str2) _strnicmp(str1, str2, strlen(str2))
#else
    #include <strings.h>
#endif

static char *wav_header(int hz, int ch, int bips, int data_bytes)
{
    static char hdr[44] = "RIFFsizeWAVEfmt \x10\0\0\0\1\0ch_hz_abpsbabsdatasize";
    unsigned long nAvgBytesPerSec = bips*ch*hz >> 3;
    unsigned int nBlockAlign      = bips*ch >> 3;

    *(int *  )(void*)(hdr + 0x04) = 44 + data_bytes - 8;   /* File size - 8 */
    *(short *)(void*)(hdr + 0x14) = 1;                     /* Integer PCM format */
    *(short *)(void*)(hdr + 0x16) = ch;
    *(int *  )(void*)(hdr + 0x18) = hz;
    *(int *  )(void*)(hdr + 0x1C) = nAvgBytesPerSec;
    *(short *)(void*)(hdr + 0x20) = nBlockAlign;
    *(short *)(void*)(hdr + 0x22) = bips;
    *(int *  )(void*)(hdr + 0x28) = data_bytes;
    return hdr;
}
static unsigned char *preload(FILE *file, int *data_size)
{
    unsigned char *data;
    *data_size = 0;
    if (!file)
        return 0;
    fseek(file, 0, SEEK_END);
    *data_size = (int)ftell(file);
    fseek(file, 0, SEEK_SET);
    data = (unsigned char*)malloc(*data_size);
    if (!data)
        return 0;
    if ((int)fread(data, 1, *data_size, file) != *data_size)
        exit(1);
    return data;
}

static void decode_file(const unsigned char *buf_mp3, int mp3_size, FILE *file_out, const int wave_out)
{
    static mp3dec_t mp3d;
    mp3dec_frame_info_t info;
    int data_bytes, samples, total_samples = 0, maxdiff = 0;
    double MSE = 0.0, psnr;

    mp3dec_init(&mp3d);
    memset(&info, 0, sizeof(info));
    if (wave_out && file_out)
        fwrite(wav_header(0, 0, 0, 0), 1, 44, file_out);
    do
    {
        short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
        samples = mp3dec_decode_frame(&mp3d, buf_mp3, mp3_size, pcm, &info);
        if (samples)
        {
            total_samples += samples*info.channels;
            if (file_out)
                fwrite(pcm, samples, 2*info.channels, file_out);
        }
        buf_mp3  += info.frame_bytes;
        mp3_size -= info.frame_bytes;
    } while (info.frame_bytes);
    MSE /= total_samples ? total_samples : 1;
    if (0 == MSE)
        psnr = 99.0;
    else
        psnr = 10.0*log10(((double)0x7fff*0x7fff)/MSE);
    printf("rate=%d samples=%d max_diff=%d PSNR=%f\n", info.hz, total_samples, maxdiff, psnr);
    if (psnr < 96)
    {
        printf("PSNR compliance failed\n");
        exit(1);
    }
    if (wave_out && file_out)
    {
        data_bytes = ftell(file_out) - 44;
        rewind(file_out);
        fwrite(wav_header(info.hz, info.channels, 16, data_bytes), 1, 44, file_out);
    }
}

int main(int argc, char *argv[])
{
    int wave_out = 0, mp3_size;
    char *output_file_name = (argc >= 2) ? argv[2] : NULL;
    FILE *file_out = NULL;
    if (output_file_name)
    {
        file_out = fopen(output_file_name, "wb");
        char *ext = strrchr(output_file_name, '.');
        if (ext && !strcasecmp(ext + 1, "wav"))
            wave_out = 1;
    }
    char *input_file_name  = (argc > 1) ? argv[1] : NULL;
    if (!input_file_name)
    {
        printf("error: no file names given\n");
        return 1;
    }
    FILE *file_mp3 = fopen(input_file_name, "rb");
    unsigned char *buf_mp3 = preload(file_mp3, &mp3_size);
    if (file_mp3)
        fclose(file_mp3);
    if (!buf_mp3 || !mp3_size)
    {
        printf("error: no input data\n");
        return 1;
    }
    decode_file(buf_mp3, mp3_size, file_out, wave_out);
    if (buf_mp3)
        free(buf_mp3);
    if (file_out)
        fclose(file_out);
    return 0;
}

