#include <compressedFile.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

FILE *fileopen(const char *filename, const char *mode){
    FILE *file;
    #ifdef _MSC_VER
        errno_t err = fopen_s(&file, filename, mode);
        if (err){
            file = NULL;
        }
    #else
        file = fopen(filename, mode);
    #endif
    return file;
}

//29 bits per round plus extra 32 bits per seed rounded up to the next byte
//number of rounds is determined at runtime but is probably 141 to 1000 = 860 rounds
//and number of seeds compressed together is also determined at runtime but should be 10
int createCompressedFileSeeds(CompressedFile *file, const char *filename, const char *mode, const int numRounds, const int numSeeds){
    const size_t size = (7ULL + (96ULL + 33ULL * numRounds) * numSeeds) / 8ULL;
    return createCompressedFile(file, filename, mode, size);
}

int createCompressedFile(CompressedFile *file, const char *filename, const char *mode, const size_t size){
    if (!file){
        return NULL_PTR_FAIL;
    }
    file->buffersize = size;
    if ((!filename) || (!mode)){
        file->file = NULL;
    } else {
        file->file = fileopen(filename, mode);
        if (!file->file){
            return FILE_FAIL;
        }
    }
    file->index = 0;
    file->buffer = malloc(file->buffersize);
    if (!file->buffer){
        return MALLOC_FAIL;
    }
    memset(file->buffer, 0, file->buffersize);
    return 0;
}

int setCompressedFilename(CompressedFile *file, const char *filename, const char *mode){
    if (file->file){
        fclose(file->file);
        file->file = NULL;
    }
    file->file = fileopen(filename, mode);
    if (!file->file){
        return FILE_FAIL;
    }
    return 0;
}

void freeCompressedFile(CompressedFile *file){
    if (!file) return;
    if (file->file){
        fclose(file->file);
        file->file = NULL;
    }
    if (file->buffer){
        free(file->buffer);
        file->buffer = NULL;
        file->buffersize = 0;
        file->index = 0;
    }
}

#if defined _WIN32

#include <windows.h>
#include <conio.h>

int ensureDirectoryExists(const char *path){
    DWORD attrs = GetFileAttributesA(path);
    if (attrs == INVALID_FILE_ATTRIBUTES){
        if (CreateDirectoryA(path, NULL)){
            return 0;
        } else {
            //creation failed
            return FILE_FAIL;
        }
    }
    if (attrs & FILE_ATTRIBUTE_DIRECTORY){
        return 0;
    }
    //something else already exists with the same name
    return FILE_FAIL;
}

int32_t getInput(void) {
    if (_kbhit()) {
        return _getch();
    }
    return EOF;
}

#elif defined __unix || defined __APPLE__

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

int ensureDirectoryExists(const char *path) {
    struct stat st;

    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return 0;
        } else {
            return FILE_FAIL;
        }
    }

    if (errno != ENOENT) {
        perror("stat: ");
        return FILE_FAIL;
    }

    if (mkdir(path, 0755) == 0) {
        return 0;
    } else {
        perror("mkdir: ");
        return FILE_FAIL;
    }
}

int32_t getInput(void) {
    struct termios oldt, newt;
    int32_t ch;
    int oldf;
    
    //store terminal settings and make getchar non blocking
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    //restore previous terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    return ch; //if ch == EOF return EOF is redunant
}

#else
#error "OS not supported"
#endif

int dumpBufferToFileDefault(CompressedFile *file){
    return dumpBufferToFile(file, Z_DEFAULT_COMPRESSION);
}

int dumpBufferToFile(CompressedFile *file, int compressionLevel){
    if (file->file == NULL){
        return FILE_FAIL;
    }
    size_t currentNumBytes = (file->index + 7) / 8;
    size_t numBytesOut = currentNumBytes + (currentNumBytes / 2) + 12;
    Byte *compressedData = malloc(numBytesOut);
    size_t compressedLen;
    FILE *fptr = file->file;

    z_stream stream = {.zalloc=Z_NULL, .zfree=Z_NULL, .opaque=Z_NULL,
                       .next_in=(Bytef*)file->buffer, .next_out=compressedData,
                       .avail_in=(uInt)currentNumBytes, .avail_out=(uInt)numBytesOut};

    if (deflateInit(&stream, compressionLevel) != Z_OK) {
        fprintf(stderr, "Error: Unable to initialize zlib stream.\n");
        free(compressedData);
        return MISC_FAIL;
    }

    if (deflate(&stream, Z_FINISH) != Z_STREAM_END) {
        fprintf(stderr, "Error: Unable to compress data.\n");
        deflateEnd(&stream);
        free(compressedData);
        return MISC_FAIL;
    }

    deflateEnd(&stream);
    compressedLen = numBytesOut - stream.avail_out;

    if (fwrite(&compressedLen, sizeof(size_t), 1, fptr) != 1){
        fprintf(stderr, "Error: Unable to write data.\n");
        return FILE_FAIL;
    }
    if (fwrite(compressedData, 1, compressedLen, fptr) != compressedLen){
        fprintf(stderr, "Error: Unable to write data.\n");
        return FILE_FAIL;
    }
    //for (i = 0; i < compressedLen; i++){
    //    fwrite(&compressedData[i], 1, 1, fptr);
    //}

    file->index=0;
    memset(file->buffer, 0, file->buffersize);
    free(compressedData);

    return 0;
}

int readBufferFromFile(CompressedFile *file){
    if (!file->file){
        return FILE_FAIL;
    }
    FILE *fptr = file->file;
    size_t size;
    if (fread(&size, sizeof(size_t), 1, fptr) != 1){
        fprintf(stderr, "Error: Unable to read data.\n");
        return FILE_FAIL;
    }
    Byte *buffer = malloc(size);
    if (fread(buffer, 1, size, fptr) != size){
        fprintf(stderr, "Error: Unable to read data.\n");
        return FILE_FAIL;
    }
    
    z_stream stream = {.zalloc=Z_NULL, .zfree=Z_NULL, .opaque=Z_NULL,
        .next_in=(Bytef*)buffer, .next_out=file->buffer,
        .avail_in=(uInt)size, .avail_out=(uInt)file->buffersize};

    if (inflateInit(&stream) != Z_OK) {
        fprintf(stderr, "Error: Unable to initialize zlib stream.\n");
        free(buffer);
        return MISC_FAIL;
    }

    if (inflate(&stream, Z_FINISH) != Z_STREAM_END){
        fprintf(stderr, "Error: Unable to decompress data.\n");
        inflateEnd(&stream);
        return MISC_FAIL;
    }
    file->index = 0;

    return 0;
}

/**
 * @param bufferSize - the size of the buffer in bytes
 * @param shift - 0 < shift < 8, how many bits to shift the buffer up
 * eg, 0xaa 0xaa << 1 -> 0x55 0x54 +
 * @return
 * 0x01 - up to 8 bits that fell off the top.
 */
unsigned char shiftBuffer(unsigned char *buffer, const size_t bufferSize, const unsigned char shift){
    unsigned char tmp_a = 0, tmp_b;
    size_t i;
    for (i = 0; i < bufferSize; i++){
        tmp_b = buffer[i];
        buffer[i] = (buffer[i] << shift) + tmp_a;
        tmp_a = tmp_b >> (8-shift);
    }
    return tmp_a;
} 

int writeToBuffer56(CompressedFile *file, uint64_t data, const uint32_t nbits){
    if ((nbits + file->index) >= (file->buffersize * 8)){
        fprintf(stderr, "out of space to write to buffer");
        return MISC_FAIL;
    }
    if (nbits > 56){
        fprintf(stderr, "the number of bits must not excede 56, due to bit manipulation requirements");
        return MISC_FAIL;
    }
    if (nbits == 0){
        return 0;
    }
    //shift the data up so the first bit aligns with the first bit of the file buffer
    {
        size_t mask = (2LLU << (nbits - 1LLU)) - 1LLU;
        data &= mask;
    }
    size_t nbytes, i, index;
    nbytes = (nbits + (file->index % 8) + 7) / 8;
    data <<= file->index % 8;
    index = file->index / 8;
    file->buffer[index] += data & 0xff;
    for (i = 1; i < nbytes; i++){
        data >>= 8;
        file->buffer[index + i] += data & 0xff;
    }
    //file->buffer[index + nbytes] += extra;
    file->index += nbits;
    return 0;
}

void _writeToBufferNoShift(CompressedFile *file, const Byte *data, const uint32_t nbits){
    Byte top_mask = (1 << (nbits % 8)) - 1;
    for (size_t index = 0; index < nbits / 8; index++){
        file->buffer[(file->index / 8) + index] = data[index];
    }
    Byte currentData = data[nbits / 8];
    file->buffer[(file->index / 8) + (nbits / 8)] = currentData & top_mask;
    file->index += nbits;
}

/**
 * @param data a reference to the data to be written to the file, ** this function is destructive to the data **
 */
int writeToBuffer(CompressedFile *file, const Byte *data, const uint32_t nbits){
    if ((nbits + file->index) >= (file->buffersize * 8)){
        fprintf(stderr, "out of space to write to buffer");
        return MISC_FAIL;
    }
    if (file->index % 8 == 0){
        _writeToBufferNoShift(file, data, nbits);
        return 0;
    }
    Byte rem_bits = nbits;
    Byte up = file->index % 8;
    Byte down = 8 - up;
    Byte mask = 0xff;
    if (rem_bits < 8){
        mask = (1 << rem_bits) - 1;
    }
    uint16_t current_data = data[0] & mask;
    current_data <<= up;
    rem_bits -= down;
    //shift the data up so the first bit aligns with the first bit of the file buffer
    file->buffer[file->index / 8] += current_data & 0xff;
    file->buffer[file->index / 8 + 1] += (current_data & 0xff00) >> 8;
    for (uint32_t index = 1; index <= nbits / 8; index++){
        if (rem_bits < 8){
            mask = (1 << rem_bits) - 1;
        }
        current_data = data[index] & mask;
        current_data <<= up;
        file->buffer[(file->index / 8) + index] += current_data & 0xff;
        file->buffer[(file->index / 8) + index + 1] += (current_data & 0xff00) >> 8; 
        rem_bits -= 8;
    }
    file->index += nbits;
    return 0;
}

int readFromBufferedFile56(CompressedFile *file, uint64_t *data, const uint32_t nbits){
    if ((nbits + file->index) >= (file->buffersize * 8)){
        fprintf(stderr, "out of space to read from buffer");
        return MISC_FAIL;
    }
    if (nbits > 56){
        fprintf(stderr, "the number of bits must not excede 56, due to bit manipulation requirements");
        return MISC_FAIL;
    }
    if (nbits == 0){
        *data = 0;
        return 0;
    }
    const uint64_t downshift_required = file->index % 8;
    const uint64_t datamask = (1LLU << (uint64_t)nbits) - 1LLU;
    for (size_t index = 0; index <= (nbits + (file->index % 8) + 7) / 8; index++){
        const uint64_t byte = file->buffer[(file->index / 8) + index];
        *data += byte << (index * 8LLU);
    }
    *data >>= downshift_required;
    *data &= datamask;
    file->index += nbits;
    return 0;
}

void _readFromBufferNoShift(CompressedFile *file, Byte *data, const uint32_t nbits){
    Byte top_mask = (1 << (nbits % 8)) - 1;
    for (size_t index = 0; index < nbits / 8; index++){
        data[index] = file->buffer[(file->index / 8) + index];
    }
    data[nbits / 8] = file->buffer[(file->index / 8) + ((nbits + 7) / 8)] & top_mask;
    file->index += nbits;
}

int readFromBufferedFile(CompressedFile *file, Byte *data, const uint32_t nbits){
    if ((nbits + file->index) >= (file->buffersize * 8)){
        fprintf(stderr, "out of space to read from buffer");
        return MISC_FAIL;
    }
    if (file->index % 8 == 0){
        _readFromBufferNoShift(file, data, nbits);
        return 0;
    }
    Byte rem_bits = nbits;
    Byte down = file->index % 8;
    Byte up = 8 - down;
    Byte mask = 0xff;
    if (rem_bits < 8){
        mask = (1 << rem_bits) - 1;
    }
    uint16_t current_data = file->buffer[file->index / 8] & mask;
    data[0] = current_data >> down;
    rem_bits -= up;
    for (size_t index = 1; index <= nbits / 8; index++){
        if (rem_bits < 8){
            mask = (1 << rem_bits) - 1;
        }
        current_data = file->buffer[file->index / 8 + index] & mask;
        current_data <<= up;
        data[index - 1] += current_data & 0xff;
        data[index] += (current_data & 0xff00) >> 8; 
        rem_bits -= 8;
    }
    file->index += nbits;

    return 0;
}

/*
int main(){
    unsigned char d[2] = {0xaa, 0xaa};
    printf("%hhx, %hhx\n", d[1], d[0]);
    unsigned char extra = shiftBuffer(d, 2, 1);
    printf("%hhx, %hhx, %hhx\n", extra, d[1], d[0]);
    return 0;
}
*/
