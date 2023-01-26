//
// Created by mlgsk on 21.01.2023.
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
enum memoryBlockType { available, usedNext, usedFirst };


struct inode {
    unsigned int flag;
    size_t size;
    char name[FILENAME_SIZE];
    int next_inode;
};
/*Only using when program is running*/
struct FileSystem {
    struct FileSystemData fileSystemData;
    FILE * file;
    struct inode * inodesList;
    int inode_num;
};

/* Function needed */
struct FileSystem* createFileSystem(size_t chosenFileSystemSize);  /* Creates Virtual Disk */
void uploadFileToFileSystem(struct FileSystem* ourFileSystem, const char* fileName);  /*Add File to Virtual Disk */
void copyFileFromVFS(struct FileSystem* myFileSystem, const char*, const char* fileToExport);  /* Copy file from Virtual Disk to minix disk */
void removeFileFromVFS(struct FileSystem* myFileSystem, const char* fileToDelete);  /* Remove file from Virtual Disk */
void destroyVFS();  /* Delete virtual disk */
void listFiles(struct FileSystem* myFileSystem);  /* !Shows file */
void diskStatistics(struct FileSystem* myFileSystem);  /*  !Shows virtual disk statistics */

/*Help function*/
struct FileSystem* openFileSystem();  /* Open virtual disk */
void saveChangesAndCloseFileSystem(struct FileSystem* ourFileSystem);  /* Close Virtual disk */
unsigned int calculateNumberOfInodes(size_t fileSystemSize);  /*Gets how manny inodes and blocks we can allocate*/
void closeWithoutSaving(struct FileSystem* ourFileSystem);  /**/
unsigned int getReqInodes(size_t);  /**/
unsigned int getBlockOffset(unsigned int, unsigned int); /**/



#endif //LAB_6_FILE_SYSTEM_FILESYSTEM_H
