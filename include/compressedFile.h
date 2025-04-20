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

/**
 * checks if a directory exists, if it doesn't exist, create it
 * @returns 
 *  - 0 if the directory exists
 * 
 *  - FILE_FAIL (non-zero) if the directory could not be created
 */
int ensureDirectoryExists(const char *directoryName);

/**
 * @param bufferSize - the size of the buffer in bytes
 * @param shift - 0 < shift < 8, how many bits to shift the buffer up
 * eg, 0xaa 0xaa << 1 -> 0x55 0x54 +
 * @return
 * 0x01 - up to 8 bits that fell off the top.
 */
unsigned char shiftBuffer(unsigned char *buffer, const size_t bufferSize, const unsigned char shift);

typedef struct CompressedFile {
    FILE *file;
    Byte *buffer;
    size_t buffersize, index;
} CompressedFile;

/**
 * Initialises a CompressedFile to hae a Byte buffer large enough to store all the fbads, bads, cash and seeds for some number of rounds across some number of seeds
 * Initialises a CompressedFile
 * @param mode the mode with which to open the file, this must be a binary mode, such as "wb" "rb" or "ab"
 * @param numRounds the number of rounds which are being stored for each seed
 * @param numSeeds the number of seeds that should be stored in each buffer
 * @returns
 *  - 0 on success
 * 
 *  - FILE_FAIL (non-zero) if the file specified could not be opened
 * 
 *  - MALLOC_FAIL (non-zero) if allocating the Byte buffer was not successful
 */
int CompressedFile_InitSeeds(CompressedFile *file, const char *filename, const char *mode, const int numRounds, const int numSeeds);
/**
 * Initialises a CompressedFile
 * @param mode the mode with which to open the file, this must be a binary mode, such as "wb" "rb" or "ab"
 * @param size the minimum size of the Byte buffer that should be created
 * @returns
 *  - 0 on success
 * 
 *  - FILE_FAIL (non-zero) if the file specified could not be opened
 * 
 *  - MALLOC_FAIL (non-zero) if allocating the Byte buffer was not successful
 */
int CompressedFile_Init(CompressedFile *file, const char *filename, const char *mode, const size_t size);
/**
 * Changes the file/mode of a CompressedFile
 * @param mode the mode with which to open the file, this must be a binary mode, such as "wb" "rb" or "ab"
 * @returns
 *  - 0 on success
 * 
 *  - FILE_FAIL (non-zero) if the file specified could not be opened
 */
int CompressedFile_SetFilename(CompressedFile *file, const char *filename, const char *mode);
/**
 * Frees the Byte buffer and closes the file held by the CompressedFile
 * @returns
 *  - 0 on success
 * 
 *  - FILE_FAIL (non-zero) if the fclose call returned an error (likely the last buffered write failed)
 */
int CompressedFile_Free(CompressedFile *file);
/**
 * Compresses the Byte buffer using zlib's default compression, and writes the compressed ouptut to the file
 * @param file a CompressedFile initalized to write mode - mode="wb" or "ab"
 * @returns
 *  - 0 on success
 * 
 *  - MALLOC_FAIL (non-zero) if allocating the output buffer fails
 * 
 *  - MISC_FAIL (non-zero) if there was an error with zlib compressing the buffer
 * 
 *  - FILE_FAIL (non-zero) if the file could not be written to
 */
int CompressedFile_FlushDefault(CompressedFile *file);
/**
 * Compresses the Byte buffer using specified zlib compression level, and writes the compressed ouptut to the file
 * @param file a CompressedFile initalized to write mode - mode="wb" or "ab"
 * @returns
 *  - 0 on success
 * 
 *  - MALLOC_FAIL (non-zero) if allocating the output buffer fails
 * 
 *  - MISC_FAIL (non-zero) if there was an error with zlib compressing the buffer
 * 
 *  - FILE_FAIL (non-zero) if the file could not be written to
 */
int CompressedFile_Flush(CompressedFile *file, const int compressionLevel);
/**
 * @returns the number of empty bits in the buffer that can be written to / are left to read
 * 
 * this function uses the allocated buffer size, disregarding the actual size of the decompressed buffer
 */
size_t CompressedFile_RemainingBits(const CompressedFile *file);
/**
 * Skips over the next buffer when reading the file
 * @param file a CompressedFile initialized to read mode - mode="rb"
 * @returns
 *  - 0 on success
 * 
 *  - FILE_FAIL (non-zero) if the file could not be read
 */
int CompressedFile_SkipNextBuffer(CompressedFile *file);
/**
 * Reads and decompresses the next buffer when reading the file
 * @param file a CompressedFile initialized to read mode - mode="rb"
 * @returns
 *  - 0 on success
 * 
 *  - MALLOC_FAIL (non-zero) if allocating the input buffer fails
 * 
 *  - MISC_FAIL (non-zero) if there was an error with zlib decompressing the buffer
 * 
 *  - FILE_FAIL (non-zero) if the file could not be read
 */
int CompressedFile_LoadNextBuffer(CompressedFile *file);
/**
 * Shfits the index at which the Byte buffer is being read from / written to
 * @returns
 *  - 0 on success
 * 
 *  - MISC_FAIL (non-zero) if the shift would move the index out of the bounds of the byte buffer
 */
int CompressedFile_ShiftBufferIndex(CompressedFile *file, const int32_t shift);
/**
 * Writes up to 56 bits of data to the Byte buffer
 * @returns
 *  - 0 on success
 * 
 *  - MISC_FAIL (non-zero) if the data could not be written to the buffer
 */
int CompressedFile_Write56(CompressedFile *file, uint64_t data, const uint32_t nbits);
/**
 * Writes data to the Byte buffer
 * @returns
 *  - 0 on success
 *  
 *  - MISC_FAIL (non-zero) if the data could not be written to the buffer
 */
int CompressedFile_Write(CompressedFile *file, const Byte *data, const uint32_t nbits);
/**
 * Reads up to 56 bits of data from the Byte buffer
 * @param data pointer to the int to fill with the data
 * @returns
 *  - 0 on success
 * 
 *  - MISC_FAIL (non-zero) if the data could not be read from the buffer
 */
int CompressedFile_Read56(CompressedFile *file, uint64_t *data, const uint32_t nbits);
/**
 * Reads data from the Byte buffer
 * @param data pointer to the buffer to fill with the data
 * @returns
 *  - 0 on success
 * 
 *  - MISC_FAIL (non-zero) if the data could not be read from the buffer
 */
int CompressedFile_Read(CompressedFile *file, Byte *data, uint32_t nbits);

