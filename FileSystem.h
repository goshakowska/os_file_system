//
// Created by mlgsk on 21.01.2023.
//

#ifndef LAB_6_FILE_SYSTEM_FILESYSTEM_H
#define LAB_6_FILE_SYSTEM_FILESYSTEM_H

#pragma once
#include <stdio.h>
#define BLOCK 1024
#define FILENAME 32


/*Virtual Disk Data*/
struct superblock {
    size_t size;
    size_t free_size;
    unsigned int file_number;
// unsigned int first_file; /*Number of inode thats points to the first file*/
};

/*Block data*/
enum flag_type { FREE, USED, FIRST };
struct inode {
    unsigned int flag;
    size_t size;
    char name[FILENAME];
    int next_inode;
};
/*Only using when program is running*/
struct VFS {
    struct superblock SB;
    FILE * FP;
    struct inode * inode_list;
    int inode_num;
};

/* Function needed */
struct VFS* CreateVFS(size_t);  /* Creates Virtual Disk */
void addFileToVFS(struct VFS*, const char*);  /*Add File to Virtual Disk */
void copyFileFromVFS(struct VFS*, const char*, const char*);  /* Copy file from Virtual Disk to minix disk */
void removeFileFromVFS(struct VFS*, const char*);  /* Remove file from Virtual Disk */
void destroyVFS();  /* Delete virtual disk */
void ls(struct VFS*);  /* !Shows file */
void diskStatistics(struct VFS*);  /*  !Shows virtual disk statistics */

/*Help function*/
struct VFS* openVFS();  /* Open virtual disk */
void closeAndSaveVFS(struct VFS*);  /* Close Virtual disk */
unsigned int get_number_inodes(size_t);  /*Gets how manny inodes and blocks we can allocate*/
void closeWithoutSaving(struct VFS*);  /**/
unsigned int getReqInodes(size_t);  /**/
unsigned int getBlockOffset(unsigned int, unsigned int); /**/



#endif //LAB_6_FILE_SYSTEM_FILESYSTEM_H
