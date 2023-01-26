//
// Created by Malgorzata Kozlowska on 21.01.2023.
//

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "FileSystem.h"
#include <errno.h>


const char * fileSystemName = "FileSystem";  // our File System name


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
        printf("ERROR: Przyznano za malo pamieci do utworzenia systemu plikow.\n");
        exit(-1);
    }

    fileSystem->file = fopen(fileSystemName, "wb");

    /* Check if the file has been created properly */
    if (!fileSystem->file)
    {
        free(fileSystem);
        printf("ERROR: System plikow nie zostal poprawnie utworzony.\n");
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
    fileSystem->inodeQuantity = calculateNumberOfInodes(chosenFileSystemSize);

    /* Calculate and set the size of the memory for data */
    fileSystem->fileSystemData.freeSize = fileSystem->inodeQuantity * BLOCK_SIZE;

    /* Allocate memory and set status for inodes */
    fileSystem->inodesList = malloc(sizeof(struct inode) * fileSystem->inodeQuantity);
    for (iterator = 0; iterator < fileSystem->inodeQuantity; ++iterator)
    {
        fileSystem->inodesList[iterator].inodeStatus = available;
    }

    /* File System has been created successfully */
    printf("Utworzono system plikow.\n");

    /* Save changes and close File System*/
    saveChangesAndCloseFileSystem(fileSystem);
    return fileSystem;
}



void saveChangesAndCloseFileSystem(struct FileSystem* ourFileSystem)
{
    /* Set the cursor at the beginning of the File System */
    fseek(ourFileSystem->file, 0, SEEK_SET);
    /* Save changes */
    fwrite(&ourFileSystem->fileSystemData, sizeof(struct FileSystemData), 1, ourFileSystem->file);
    fwrite(ourFileSystem->inodesList, sizeof(struct inode), ourFileSystem->inodeQuantity, ourFileSystem->file);

    /* Close File System*/
    fclose(ourFileSystem->file);

    /* Free used memory */
    free(ourFileSystem->inodesList);
    free(ourFileSystem);
    ourFileSystem = NULL;

}



void closeFileSystemWithoutSaving(struct FileSystem* ourFileSystem)
{
    if (ourFileSystem -> file)
    {
        /* Close File System*/
        fclose(ourFileSystem->file);

        /* Free used memory */
        free(ourFileSystem->inodesList);
    }
    free(ourFileSystem);
    ourFileSystem = NULL;
}



struct FileSystem* openFileSystem()
{
    struct FileSystem *ourFileSystem;

    /* Allocate data for our File System */
    ourFileSystem = malloc(sizeof(struct FileSystem));

    /* Try to open File System */
    ourFileSystem->file=fopen(fileSystemName, "r+b");
    if (!ourFileSystem->file)
    {
        closeFileSystemWithoutSaving(ourFileSystem);
        printf("ERROR: Otwarcie systemu plikow nie powiodlo sie.\n");
        exit(-1);
    }
//    fseek(ourFileSystem->file, 0, SEEK_END);

    /* Set cursor at the begging of the File System */
    fseek(ourFileSystem->file, 0, SEEK_SET);

    /* Try to read the File System */
    if (fread(&ourFileSystem->fileSystemData, sizeof(struct FileSystemData), 1, ourFileSystem->file) <= 0)
    {
        printf("ERROR: Wczytanie danych z systemu plikow nie powiodlo sie.\n");
        closeFileSystemWithoutSaving(ourFileSystem);
        exit(-1);
    }

    /* Calculate and set the number of inodes in our File System */
    ourFileSystem->inodeQuantity = calculateNumberOfInodes(ourFileSystem->fileSystemData.size);

    /* Allocate memory for inodes */
    ourFileSystem->inodesList = malloc(sizeof(struct inode) * ourFileSystem->inodeQuantity);

    /* Try to read inodes*/
    if (fread(ourFileSystem->inodesList, sizeof(struct inode), ourFileSystem->inodeQuantity, ourFileSystem->file) <= 0)
    {
        printf("ERROR: Wczytanie danych inodow nie powiodlo sie.\n");
        closeFileSystemWithoutSaving(ourFileSystem);
        exit(-1);
    }

    /* File System has been opened successfully */
    return ourFileSystem;
}



void copyFileToFileSystem(struct FileSystem *ourFileSystem, const char* fileName)
{
    /* Initialize variables for the file */
    FILE *newFile;
    size_t newFileSize;
    char buffer[BLOCK_SIZE];
    unsigned int requiredMemoryBlocks, i, j;
    unsigned int *freeMemoryBlocks;

    /* Check if there exists a file with the same name*/
    for (i = 0; i < ourFileSystem->inodeQuantity; i++)
    {
        if(ourFileSystem->inodesList[i].inodeStatus == usedStart)
        {
            if (strcmp(fileName, ourFileSystem->inodesList[i].name) == 0)
            {
                printf("ERROR: W systemie plikow istnieje plik o podanej nazwie : %s.\n", fileName);
                closeFileSystemWithoutSaving(ourFileSystem);
                exit(-1);
            }
        }
    }

    /* Try to open the provided file */
    newFile = fopen(fileName, "rb");
    if (!newFile)
    {
        printf("ERROR: Otwarcie pliku nie powiodlo się.\n");
        closeFileSystemWithoutSaving(ourFileSystem);
        exit(-1);
    }
    /* Get the size of the file that will be uploaded */
    fseek(newFile, 0, SEEK_END);
    newFileSize = ftell(newFile);

    /* Check if there is enough size for the provided file */
    if (ourFileSystem->fileSystemData.freeSize < newFileSize)
    {
        errno = ENOMEM;
        printf("ERROR: System plikow nie dysponuje odpowiednia iloscia pamieci.");
        closeFileSystemWithoutSaving(ourFileSystem);
        exit(-1);
    }


    requiredMemoryBlocks = calculateRequiredNumberOfMemoryBlocks(newFileSize);
    freeMemoryBlocks = malloc(requiredMemoryBlocks * sizeof(unsigned int));
    /* Assign memory blocks for the file that is to be uploaded */
    for (i = 0, j = 0; j < requiredMemoryBlocks; i++)
    {
        if (ourFileSystem->inodesList[i].inodeStatus == available)
        {
            /* Save the position (id) of the memory block that can be used */
            freeMemoryBlocks[j] = i;
            j++;
            /* If the necessary quantity of the free memory blocks has been found - end loop */
            if (j == requiredMemoryBlocks) break;
        }
    }
    /* Could not find the necessary quantity of the memory blocks for the file. */
    if (j != requiredMemoryBlocks)
    {
        printf("ERROR: W systemie plikow nie zostala znaleziona odpowiednia liczba wolnych bloklw pamieci na zapis pliku.\n");
        free(freeMemoryBlocks);
        closeFileSystemWithoutSaving(ourFileSystem);
        fclose(newFile);
        exit(-1);
    }
    /* Save file to our File System */
    /* Set cursor at the beginning od the file */
    fseek(newFile, 0, SEEK_SET);
    for (i = 0; i < requiredMemoryBlocks; i++)
    {
        /* Set statuses for the memory blocks */
        if (i == 0)
            ourFileSystem->inodesList[freeMemoryBlocks[i]].inodeStatus = usedStart;
        else
            ourFileSystem->inodesList[freeMemoryBlocks[i]].inodeStatus = usedNext;

        /* Copy and set file name to our File System */
        strncpy(ourFileSystem->inodesList[freeMemoryBlocks[i]].name, fileName, FILENAME_SIZE);

        /* Set memory blocks*/
        /* Fill the whole memory block (if it is not the final one) */
        if (i != requiredMemoryBlocks - 1)
            ourFileSystem->inodesList[freeMemoryBlocks[i]].size = BLOCK_SIZE;
        else
        {
            /* Check if the file size is a multiple of the memory block size */
            if (!(newFileSize % BLOCK_SIZE))
                ourFileSystem->inodesList[freeMemoryBlocks[i]].size = BLOCK_SIZE;
            else
                /* Set size to the reminder of the division */
                ourFileSystem->inodesList[freeMemoryBlocks[i]].size = newFileSize % BLOCK_SIZE;
        }
        /*Next Inode*/
        if (i < requiredMemoryBlocks - 1)
            ourFileSystem->inodesList[freeMemoryBlocks[i]].nextInode = freeMemoryBlocks[i + 1];
        else
            ourFileSystem->inodesList[freeMemoryBlocks[i]].nextInode = -1;

        /*Copy from file to virtual disk*/
        fseek(ourFileSystem->file, getBlockOffset(freeMemoryBlocks[i], ourFileSystem->inodeQuantity), SEEK_SET);
        fread(buffer, 1, sizeof(buffer), newFile);
        fwrite(buffer, 1, ourFileSystem->inodesList[freeMemoryBlocks[i]].size, ourFileSystem->file);
    }

    /* Decrement File System size after uploading the provided file */
    ourFileSystem->fileSystemData.freeSize -= requiredMemoryBlocks * BLOCK_SIZE;

    /* Increment number of files the File System stores */
    ourFileSystem->fileSystemData.fileNumber +=1;

    free(freeMemoryBlocks);

    /* Close the file that has been copied to our File System */
    fclose(newFile);
    saveChangesAndCloseFileSystem(ourFileSystem);
    printf("Plik o nazwie %s zostal skopiowany do systemu plikow.\n", fileName);
}



void copyFileFromFileSystem(struct FileSystem* ourFileSystem, const char* fileName, const char* fileNameOutsideFileSystem)
{
    /* Initialize variables for the file */
    FILE * newFile;
    char buffer[BLOCK_SIZE];
    unsigned int i, firstNode;
    int found = 0;

    /* Try to find the first memory block of the provided file */
    for (i = 0; i < ourFileSystem->inodeQuantity; i++)
    {
        if (ourFileSystem->inodesList[i].inodeStatus == usedStart && strncmp(ourFileSystem->inodesList[i].name, fileName, FILENAME_SIZE) == 0)
        {
            firstNode = i;
            found = 1;
            break;
        }
    }
    /* Could not find the file data with the provided name */
    if (found==0)
    {
        printf("ERROR: Plik o nazwie %s nie zostal znaleziony w systemie plikow.\n", fileName);
        closeFileSystemWithoutSaving(ourFileSystem);
        exit(-1);
    }

    /* Try to open the new file with the provided new name */
    newFile = fopen(fileNameOutsideFileSystem, "wb");
    if (!newFile)
    {
        printf("ERROR: Otworzenie pliku do zapisu o nazwie %s nie powiodlo sie.\n", fileName);
        closeFileSystemWithoutSaving(ourFileSystem);
        exit(-1);
    }

    /* Copy the data of the given file to the file outside of our File System */
    while (1)
    {
        fseek(ourFileSystem->file, getBlockOffset(firstNode, ourFileSystem->inodeQuantity), SEEK_SET);
        fread(buffer, 1, ourFileSystem->inodesList[firstNode].size, ourFileSystem->file);
        fwrite(buffer, 1, ourFileSystem->inodesList[firstNode].size, newFile);
        if (ourFileSystem->inodesList[firstNode].nextInode == -1) break;
        firstNode = ourFileSystem->inodesList[firstNode].nextInode;
    }

    /* Close newly created file */
    fclose(newFile);

    /* Close our File System */
    closeFileSystemWithoutSaving(ourFileSystem);
    printf("Plik o nazwie %s został wyeksportowany na zewnatrz system plikow\n.", fileName);
}



void deleteFileFromFileSystem(struct FileSystem* ourFileSystem, const char* fileName)
{
    int currentInode, i, blocks;
    currentInode = -1;

    /* Find the starting memory block of the file that will be deleted */
    for (i = 0; i < ourFileSystem->inodeQuantity; i++)
    {
        if (ourFileSystem->inodesList[i].inodeStatus == usedStart)
        {
            if (strncmp(ourFileSystem->inodesList[i].name, fileName, FILENAME_SIZE) == 0)
            {
                currentInode = i;
                break;
            }
        }
    }

    /* Could not find the file with the provided name in our File System */
    if (currentInode == -1)
    {
        printf("ERROR: Plik o nazwie %s nie zostal znaleziony w systemie plikow\n", fileName);
        closeFileSystemWithoutSaving(ourFileSystem);
        exit(-1);
    }

    blocks = 0;

    /* Set memory block status to available again */
    while (1)
    {
        ourFileSystem->inodesList[currentInode].inodeStatus = available;
        blocks++;
        if (ourFileSystem->inodesList[currentInode].nextInode != -1)
        {
            currentInode = ourFileSystem->inodesList[currentInode].nextInode;
        }
        else break;
    }

    /* Increment number of available (free) files to the ones that have been earlier used by the deleted file */
    ourFileSystem->fileSystemData.freeSize += blocks * BLOCK_SIZE;

    /* Decrement number of files */
    ourFileSystem->fileSystemData.fileNumber -= 1;
    saveChangesAndCloseFileSystem(ourFileSystem);
    printf("Plik o nazwie %s zostal usuniety z systemu plikow.\n", fileName);
}



void deleteFileSystem()
{
    /* The name of the file system is deleted - it cannot be accessed therefore it is logically deleted */
    unlink(fileSystemName);
}



unsigned int calculateNumberOfInodes(size_t fileSystemSize)
{
    return (fileSystemSize - sizeof(struct FileSystemData)) / (sizeof(struct inode) + BLOCK_SIZE);
}



unsigned int calculateRequiredNumberOfMemoryBlocks(size_t fileSize)
{
    size_t sizeLeft = fileSize;
    unsigned int requiredNumberOfBlocks = 0;

    /* Find number of the needed memory blocks */
    while (1)
    {
        requiredNumberOfBlocks++;
        if (sizeLeft <= BLOCK_SIZE) break;
        if (sizeLeft > BLOCK_SIZE) sizeLeft = sizeLeft - BLOCK_SIZE;
    }
    return requiredNumberOfBlocks;
}

unsigned int getBlockOffset(unsigned int inodeNumber, unsigned int inodeQuantity)
{
    return sizeof(struct FileSystemData) + sizeof(struct inode) * inodeQuantity + BLOCK_SIZE * inodeNumber;
}



void listFiles(struct FileSystem* myFileSystem)
{
    int i = 0, j,k;
    int n_node = -1;
    size_t size = 0;
    int *which_blocks = malloc(sizeof(int) * myFileSystem->inodeQuantity);
    printf("=================================================================\n");
    printf("%-32s%-12s%s\n","File Name","Size","Used Blocks ID");
    printf("_________________________________________________________________\n");
    for (i = 0; i < myFileSystem->inodeQuantity; i++)
    {
        if (myFileSystem->inodesList[i].inodeStatus == usedStart)
        {
            printf("%-32s", myFileSystem->inodesList[i].name);
            j = 0;
            which_blocks[j] = i;
            n_node = myFileSystem->inodesList[i].nextInode;
            size = size + myFileSystem->inodesList[i].size;
            while (n_node != -1)
            {
                j++;
                which_blocks[j] = n_node;
                size = size + myFileSystem->inodesList[n_node].size;
                n_node = myFileSystem->inodesList[n_node].nextInode;
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



void fileSystemStatistics(struct FileSystem* ourFileSystem)
{
    size_t memory_wasted = 0;
    int i = 0,j,k;
    int n_inodes=ourFileSystem->inodeQuantity;
    printf("============================================================================\n");
    printf("File System Data Synopsis:  \n");
    printf("Size:                       %zu \n", ourFileSystem->fileSystemData.size);
    printf("Data Size:                  %zu\n", sizeof(struct FileSystemData) + sizeof(struct inode) * ourFileSystem->inodeQuantity);
    printf("Block Size:                 %d\n", BLOCK_SIZE);
    printf("Free:                       %zu \n", ourFileSystem->fileSystemData.freeSize);
    printf("Files:                      %d \n", ourFileSystem->fileSystemData.fileNumber);
    printf("\n");
    printf("File names:\n");

    for (i = 0; i < ourFileSystem->inodeQuantity; i++)
    {
        if (ourFileSystem->inodesList[i].inodeStatus == usedStart)
        {
            printf("%-32s\n", ourFileSystem->inodesList[i].name);
        }
    }
    printf("\n");
    printf("Block Usage:\n");
    printf("E - Empty Memory Block | S - Start Block & In Use | N - Next Block & In use\n");
    for (i = 0; i < ourFileSystem->inodeQuantity; i++)
    {
        printf("%2d:", i);
        if (ourFileSystem->inodesList[i].inodeStatus == usedStart) printf("S | ");
        else if (ourFileSystem->inodesList[i].inodeStatus == usedNext) printf("N | ");
        else printf("E | ");
        if ((i+1)%11==0 && i+1 < ourFileSystem->inodeQuantity) printf("\n");
    }
    printf("\n");
    printf("============================================================================\n");
}

