//
// Created by Malgorzata Kozlowska on 21.01.2023.
//

#ifndef LAB_6_FILE_SYSTEM_FILESYSTEM_H
#define LAB_6_FILE_SYSTEM_FILESYSTEM_H

#pragma once
#include <stdio.h>
#define BLOCK_SIZE 1024
#define FILENAME_SIZE 32

struct FileSystemData {
    size_t size;
    size_t freeSize;
    unsigned int fileNumber;
};

/*Block data*/
enum memoryBlockType { available, usedNext, usedStart };


struct inode {
    unsigned int inodeStatus;
    size_t size;
    char name[FILENAME_SIZE];
    int nextInode;
};


struct FileSystem {
    struct FileSystemData fileSystemData;
    FILE * file;
    struct inode * inodesList;
    int inodeQuantity;
};


/* Creating & deleting file system */
struct FileSystem* createFileSystem(size_t chosenFileSystemSize);
void deleteFileSystem();

/* Copying */
void copyFileToFileSystem(struct FileSystem* ourFileSystem, const char* fileName);
void copyFileFromFileSystem(struct FileSystem* ourFileSystem, const char *fileName, const char* fileNameOutsideFileSystem);

/*Deleting */
void deleteFileFromFileSystem(struct FileSystem* ourFileSystem, const char* fileName);

/* Graphic representation */
void listFiles(struct FileSystem* myFileSystem);
void fileSystemStatistics(struct FileSystem* ourFileSystem);


/* Helper functions */

/* Opening file system */
struct FileSystem* openFileSystem();

/* Closing file system */
void saveChangesAndCloseFileSystem(struct FileSystem* ourFileSystem);
void closeFileSystemWithoutSaving(struct FileSystem* ourFileSystem);

/* Necessary calculations */
unsigned int calculateNumberOfInodes(size_t fileSystemSize);
unsigned int calculateRequiredNumberOfMemoryBlocks(size_t fileSize);
unsigned int getBlockOffset(unsigned int, unsigned int);



#endif //LAB_6_FILE_SYSTEM_FILESYSTEM_H
