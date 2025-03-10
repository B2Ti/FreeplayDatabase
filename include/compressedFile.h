#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <defines.h>
#include <zlib.h>

/**
 * @return
 * earliest inputted character, if stdin is empty then return EOF
 */
int32_t getInput(void);

int ensureDirectoryExists(const char *directoryName);

typedef struct CompressedFile {
    FILE *file;
    Byte *buffer;
    size_t buffersize, index;
} CompressedFile;

int createCompressedFileSeeds(CompressedFile *file, const char *filename, const char *mode, const int numRounds, const int numSeeds);
int createCompressedFile(CompressedFile *file, const char *filename, const char *mode, const size_t size);
int setCompressedFilename(CompressedFile *file, const char *filename, const char *mode);
void freeCompressedFile(CompressedFile *file);
int dumpBufferToFileDefault(CompressedFile *file);
int dumpBufferToFile(CompressedFile *file, const int compressionLevel);
int readBufferFromFile(CompressedFile *file);
int writeToBuffer56(CompressedFile *file, uint64_t data, const uint32_t nbits);
int writeToBuffer(CompressedFile *file, const Byte *data, const uint32_t nbits);
/**
 * @param data pointer to the int to fill with the data
 */
int readFromBufferedFile56(CompressedFile *file, uint64_t *data, const uint32_t nbits);
/**
 * @param data pointer to the buffer to fill with the data
 */
int readFromBufferedFile(CompressedFile *file, Byte *data, uint32_t nbits);
/**
 * @param bufferSize the size of the buffer in bytes
 * @param shift 0 < shift < 8, how many bits to shift the buffer up
 * eg, 0xaa 0xaa << 1 -> 0x01, 0x55 0x54
 * @return up to 8 bits that fell off the top.
 */
unsigned char shiftBuffer(unsigned char *buffer, const size_t bufferSize, const unsigned char shift);
