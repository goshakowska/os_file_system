//
// Created by Małgorzata Kozłowska on 21.01.2023.
//

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "FileSystem.h"
#include <errno.h>


const char * fileSystemName = "FileSystem";


struct FileSystem * createFileSystem(size_t chosenFileSystemSize)
{
    struct FileSystem * fileSystem;
    unsigned int iterator;

    /* Allocate data for our File System */
    fileSystem = malloc(sizeof(struct FileSystem));

    /* Check if the provided size is big enough for storing import File System data*/
    if (sizeof(struct FileSystemData) > chosenFileSystemSize)
    {
        free(fileSystem);
        errno=ENOMEM;
        perror("ERROR: Przyznano za mało pamięci do utworzenia systemu plików.\n");
        exit(-1);
    }

    fileSystem->file = fopen(fileSystemName, "wb");

    /* Check if the file has been created properly */
    if (!fileSystem->file)
    {
        free(fileSystem);
        errno=ENOEXEC;
        perror("ERROR: System plików nie został poprawnie utworzony.\n");
        exit(-1);
    }


    fseek(fileSystem->file, chosenFileSystemSize - 1, SEEK_SET);
    fwrite("0", sizeof(char), 1, fileSystem->file);
    fseek(fileSystem->file, 0, SEEK_SET);

    /* Initialize File System Data*/

    /* Set File System size */
    fileSystem->fileSystemData.size = chosenFileSystemSize;
    /* Set number of files the File System stores */
    fileSystem->fileSystemData.fileNumber = 0;

    /* Calculate and set the number of inodes in our File System */
    fileSystem->inode_num = calculateNumberOfInodes(chosenFileSystemSize);

    /* Calculate and set the size of the memory for data */
    fileSystem->fileSystemData.freeSize = fileSystem->inode_num * BLOCK_SIZE;

    /* Allocate memory and set status for inodes */
    fileSystem->inodesList = malloc(sizeof(struct inode) * fileSystem->inode_num);
    for (iterator = 0; iterator < fileSystem->inode_num; ++iterator)
    {
        fileSystem->inodesList[iterator].flag = available;
    }

    /* Inform user of Success*/
    printf("Pomyslnie utworzono dysk FileSystem\n");

    /* Save changes and close File System*/
    saveChangesAndCloseFileSystem(fileSystem);
    return fileSystem;
}



void saveChangesAndCloseFileSystem(struct FileSystem* ourFileSystem)
{
    fseek(ourFileSystem->file, 0, SEEK_SET);
    fwrite(&ourFileSystem->fileSystemData, sizeof(struct FileSystemData), 1, ourFileSystem->file);
    fwrite(ourFileSystem->inodesList, sizeof(struct inode), ourFileSystem->inode_num, ourFileSystem->file);

    fclose(ourFileSystem->file);
    free(ourFileSystem->inodesList);
    free(ourFileSystem);
    ourFileSystem = NULL;

}



void closeWithoutSaving(struct FileSystem* ourFileSystem)
{
    if (ourFileSystem -> file)
    {
        fclose(ourFileSystem->file);
        free(ourFileSystem->inodesList);
    }
    free(ourFileSystem);
    ourFileSystem = NULL;
}



struct FileSystem* openFileSystem()
{
    struct FileSystem *ourFileSystem;
    ourFileSystem = malloc(sizeof(struct FileSystem));

    ourFileSystem->file=fopen(fileSystemName, "r+b");
    if (!ourFileSystem->file)
    {
        closeWithoutSaving(ourFileSystem);
        perror("ERROR: Blad przy probie otwarcia pliku.\n");
        exit(-1);
    }
    fseek(ourFileSystem->file, 0, SEEK_END);

    fseek(ourFileSystem->file, 0, SEEK_SET);
    if (fread(&ourFileSystem->fileSystemData, sizeof(struct FileSystemData), 1, ourFileSystem->file) <= 0)
    {
        perror("ERROR: Blad przy wczytaniu informacji o dysku.\n");
        closeWithoutSaving(ourFileSystem);
        exit(-1);
    }
    ourFileSystem->inode_num = calculateNumberOfInodes(ourFileSystem->fileSystemData.size);
    ourFileSystem->inodesList = malloc(sizeof(struct inode) * ourFileSystem->inode_num);
    /*Reading inodes*/
    if (fread(ourFileSystem->inodesList, sizeof(struct inode), ourFileSystem->inode_num, ourFileSystem->file) <= 0)
    {
        perror("ERROR: przy wczytywaniu INODE.\n");
        closeWithoutSaving(ourFileSystem);
        exit(-1);
    }
    return ourFileSystem;
}



void uploadFileToFileSystem(struct FileSystem *ourFileSystem, const char* fileName)
{
    FILE *newFile;
    size_t newFileSize;
    char buffer[BLOCK_SIZE];
    unsigned int requiredNodes, i, j;
    unsigned int *freeNodesList;

    for (i = 0; i < ourFileSystem->inode_num; i++)
    {
        if(ourFileSystem->inodesList[i].flag == usedFirst)
        {
            if (strcmp(fileName, ourFileSystem->inodesList[i].name) == 0)
            {
                perror("ERROR: Istnieje juz plik o takiej nazwie.");
                closeWithoutSaving(ourFileSystem);
                exit(-1);
            }
        }
    }

    newFile = fopen(fileName, "rb");
    if (!newFile)
    {
        errno = 1;
        perror("ERROR: Blad przy probie otwarcia pliku\n");
        closeWithoutSaving(ourFileSystem);
        exit(-1);
    }
    fseek(newFile, 0, SEEK_END);
    newFileSize = ftell(newFile);
    if (ourFileSystem->fileSystemData.freeSize < newFileSize)
    {
        errno = ENOMEM;
        perror("ERROR: Brak wolnej pamieci na dysku");
        closeWithoutSaving(ourFileSystem);
        exit(-1);
    }
    requiredNodes = getReqInodes(newFileSize);
    freeNodesList = malloc(requiredNodes * sizeof(unsigned int));
    for (i = 0, j = 0; i < ourFileSystem->inode_num, j < requiredNodes; i++)
    {
        if (ourFileSystem->inodesList[i].flag == available)
        {
            freeNodesList[j] = i;
            j++;
            if (j == requiredNodes) break;
        }
    }
    if (j != requiredNodes)
    {
        perror("ERROR: Nie znaleziono odpowiedniej ilosci wolnych blokow.\n");
        free(freeNodesList);
        closeWithoutSaving(ourFileSystem);
        fclose(newFile);
        exit(-1);
    }
    fseek(newFile, 0, SEEK_SET);
    for (i = 0; i < requiredNodes; i++)
    {
        /*FLAG*/
        if (i == 0)
            ourFileSystem->inodesList[freeNodesList[i]].flag = usedFirst;
        else
            ourFileSystem->inodesList[freeNodesList[i]].flag = usedNext;

        /*Name*/
        strncpy(ourFileSystem->inodesList[freeNodesList[i]].name, fileName, FILENAME_SIZE);

        /*File Size*/
        if (i != requiredNodes - 1)
            ourFileSystem->inodesList[freeNodesList[i]].size = BLOCK_SIZE;
        else
        {
            if (!(newFileSize % BLOCK_SIZE))
                ourFileSystem->inodesList[freeNodesList[i]].size = BLOCK_SIZE;
            else
                ourFileSystem->inodesList[freeNodesList[i]].size = newFileSize % BLOCK_SIZE;
        }
        /*Next INODE*/
        if (i < requiredNodes - 1)
            ourFileSystem->inodesList[freeNodesList[i]].next_inode = freeNodesList[i + 1];
        else
            ourFileSystem->inodesList[freeNodesList[i]].next_inode = -1;

        /*Copy from file to virtual disk*/
        fseek(ourFileSystem->file, getBlockOffset(freeNodesList[i], ourFileSystem->inode_num), SEEK_SET);
        fread(buffer, 1, sizeof(buffer), newFile);
        fwrite(buffer, 1, ourFileSystem->inodesList[freeNodesList[i]].size, ourFileSystem->file);
    }
    ourFileSystem->fileSystemData.fileNumber +=1;
    ourFileSystem->fileSystemData.freeSize -= requiredNodes * BLOCK_SIZE;
    free(freeNodesList);
    fclose(newFile);
    saveChangesAndCloseFileSystem(ourFileSystem);
    printf("Pomyslnie przekopiowano plik %s na dysk FileSystem.\n", fileName);
}



void copyFileFromVFS(struct FileSystem* v, const char* fname, const char* newname)
{
    FILE * new_file;
    char buffer[BLOCK_SIZE];
    unsigned int i, first_node;
    int found = 0;


    for (i = 0; i < v->inode_num; i++)
    {
        if (v->inodesList[i].flag == usedFirst && strncmp(v->inodesList[i].name, fname, FILENAME_SIZE) == 0)
        {
            first_node = i;
            found = 1;
            break;
        }
    }
    if (found==0)
    {
        perror("ERROR: Nie znaleziono pliku na dysku FileSystem.\n");
        closeWithoutSaving(v);
        exit(-1);
    }
    new_file = fopen(newname, "wb");
    if (!new_file)
    {
        perror("ERROR: Blad przy probie otworzenia pliku do zapisu.\n");
        closeWithoutSaving(v);
        exit(-1);
    }
    while (1)
    {
        fseek(v->file, getBlockOffset(first_node, v->inode_num), SEEK_SET);
        fread(buffer, 1, v->inodesList[first_node].size, v->file);
        fwrite(buffer, 1, v->inodesList[first_node].size, new_file);
        if (v->inodesList[first_node].next_inode == -1) break;
        first_node = v->inodesList[first_node].next_inode;
    }
    fclose(new_file);
    closeWithoutSaving(v);
    printf("Pomyslnie przekopiowano plik %s z dysku FileSystem\n.", fname);
}



void removeFileFromVFS(struct FileSystem* v, const char* fname)
{
    int current_inode, i, blocks;
    current_inode = -1;
    for (i = 0; i < v->inode_num; i++)
    {
        if (v->inodesList[i].flag == usedFirst)
        {
            if (strncmp(v->inodesList[i].name, fname, FILENAME_SIZE) == 0)
            {
                current_inode = i;
                break;
            }
        }
    }
    if (current_inode == -1)
    {
        perror("ERROR: Nie znaleziono pliku na dysku FileSystem\n");
        closeWithoutSaving(v);
        exit(-1);
    }

    blocks = 0;
    while (1)
    {
        v->inodesList[current_inode].flag = available;
        blocks++;
        if (v->inodesList[current_inode].next_inode != -1)
        {
            current_inode = v->inodesList[current_inode].next_inode;
        }
        else break;
    }
    v->fileSystemData.freeSize += blocks * BLOCK_SIZE;
    v->fileSystemData.fileNumber -= 1;
    saveChangesAndCloseFileSystem(v);
    printf("Pomyslnie usunieto plik %s z dysku FileSystem\n",fname);
}



void destroyVFS()
{
    unlink(fileSystemName);
}



unsigned int calculateNumberOfInodes(size_t fileSystemSize)
{
    return (fileSystemSize - sizeof(struct FileSystemData)) / (sizeof(struct inode) + BLOCK_SIZE);
}



unsigned int getReqInodes(size_t size)
{
    size_t size_left = size;
    unsigned int req = 0;

    while (1)
    {
        req++;
        if (size_left <= BLOCK_SIZE) break;
        if (size_left > BLOCK_SIZE) size_left = size_left - BLOCK_SIZE;
    }
    return req;
}

unsigned int getBlockOffset(unsigned int num_inode, unsigned int inode_num)
{
    return sizeof(struct FileSystemData) + sizeof(struct inode) * inode_num + BLOCK_SIZE * num_inode;
}



void listFiles(struct FileSystem* myFileSystem)
{
    int i = 0, j,k;
    int n_node = -1;
    size_t size = 0;
    int *which_blocks = malloc(sizeof(int) * myFileSystem->inode_num);
    printf("=================================================================\n");
    printf("%-32s%-12s%s\n","File Name","Size","Used Blocks ID");
    for (i = 0; i < myFileSystem->inode_num; i++)
    {
        if (myFileSystem->inodesList[i].flag == usedFirst)
        {
            printf("%-32s", myFileSystem->inodesList[i].name);
            j = 0;
            which_blocks[j] = i;
            n_node = myFileSystem->inodesList[i].next_inode;
            size = size + myFileSystem->inodesList[i].size;
            while (n_node != -1)
            {
                j++;
                which_blocks[j] = n_node;
                size = size + myFileSystem->inodesList[n_node].size;
                n_node = myFileSystem->inodesList[n_node].next_inode;
            }
            printf("%-12zu", size);
            for (k = 0; k <= j; k++) printf("%d ", which_blocks[k]);
            printf("\n");
            size = 0;
        }
    }
    printf("=================================================================\n");
    free(which_blocks);
}



void diskStatistics(struct FileSystem* v)
{
    size_t memory_wasted = 0;
    int i = 0,j,k;
    int n_inodes=v->inode_num;
    printf("=================================================================\n");
    printf("*FileSystem Data:\n");
    printf("Size                  : %zu \n", v->fileSystemData.size);
    printf("Data Size             : %zu\n", sizeof(struct FileSystemData) + sizeof(struct inode) * v->inode_num);
    printf("Block Size            : %d\n", BLOCK_SIZE);
    printf("Free                  : %zu \n", v->fileSystemData.freeSize);
    printf("Files                 : %d \n", v->fileSystemData.fileNumber);
    for (i = 0; i < v->inode_num; i++)
    {
        if (v->inodesList[i].flag == usedFirst)
        {
            j = v->inodesList[i].next_inode;
            k = i;
            while (j != -1)
            {
                k = j;
                j = v->inodesList[j].next_inode;
            }
            memory_wasted = memory_wasted + (BLOCK_SIZE - v->inodesList[k].size);
        }
    }
    printf("External Fragmentation: %zu\n", v->fileSystemData.size - (v->inode_num * (BLOCK_SIZE + sizeof(struct inode)) + sizeof(struct FileSystemData)) );
    printf("Internal Fragmentation: %zu\n", memory_wasted);
    printf("Block Usage:\n");
    printf("E - Empty | F - First & In Use | U - In use\n");
    for (i = 0; i < v->inode_num; i++)
    {
        printf("%3d:", i);
        if (v->inodesList[i].flag == usedFirst) printf("F | ");
        else if (v->inodesList[i].flag == usedNext) printf("U | ");
        else printf("E | ");
        if ((i+1)%15==0 && i+1<v->inode_num) printf("\n");
    }
    printf("\n");
    printf("=================================================================\n");
}

