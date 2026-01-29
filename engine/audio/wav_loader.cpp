

#include <stdio.h>
#include <stdlib.h>

void load_wav(const char *path)
{
    FILE *wav_file = fopen(path, "rb");  // stupid windows

    if (!wav_file)
    {
        printf("wav file not found: %s\n", path);
        perror("Error");
        return ;
    }

    fseek(wav_file, 0, SEEK_END);
    long file_size = ftell(wav_file);
    char *wav_data = (char*)malloc(file_size + 1);
    fseek(wav_file, 0, SEEK_SET);
    fread(wav_data, sizeof(char), file_size, wav_file);
    wav_data[file_size] = '\0';

    
}

