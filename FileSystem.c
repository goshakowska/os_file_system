//
// Created by mlgsk on 21.01.2023.
//

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "FileSystem.h"
#include <errno.h>
const char * vfsname = "VFS";
/*=====================================================================*/
/*=====================================================================*/
struct VFS * CreateVFS(size_t user_size)
{
    struct VFS * vfs;
    vfs = malloc(sizeof(struct VFS));
    unsigned int i;

    if (sizeof(struct superblock) > user_size)
    {
        free(vfs);
        errno=ENOMEM;
        perror("[ERROR] Za malo pamieci do utworzenia dysku.\n");
        exit(1);
    }

    vfs->FP = fopen(vfsname, "wb");
    if (!vfs->FP)
    {
        free(vfs);
        perror("[ERROR] Blad przy probie utworzenia pliku.\n");
        exit(1);
    }
    fseek(vfs->FP, user_size-1, SEEK_SET);
    fwrite("0", sizeof(char), 1, vfs->FP);
    fseek(vfs->FP,0, SEEK_SET);
    /*Init superblock data*/
    vfs->SB.size = user_size;
    vfs->SB.file_number = 0;

    /*Init vfs data*/
    vfs->inode_num = get_number_inodes(user_size);
    vfs->SB.free_size = vfs->inode_num*BLOCK;
    vfs->inode_list = malloc(sizeof(struct inode) * vfs->inode_num);
    for (i = 0; i < vfs->inode_num; ++i)
    {
        vfs->inode_list[i].flag = FREE;
    }
    printf("Pomyslnie utworzono dysk VFS\n");
    closeAndSaveVFS(vfs);
    return vfs;
}
/*=====================================================================*/
/*=====================================================================*/
void closeAndSaveVFS(struct VFS* v)
{
    fseek(v->FP, 0, SEEK_SET);
    fwrite(&v->SB, sizeof(struct superblock), 1, v->FP);
    fwrite(v->inode_list, sizeof(struct inode), v->inode_num, v->FP);

    fclose(v->FP);
    free(v->inode_list);
    free(v);
    v = NULL;

}
/*=====================================================================*/
/*=====================================================================*/
void closeWithoutSaving(struct VFS* v)
{
    if (v -> FP)
    {
        fclose(v->FP);
        free(v->inode_list);
    }
    free(v);
    v = NULL;
}
/*=====================================================================*/
/*=====================================================================*/
struct VFS* openVFS()
{
    struct VFS *v;
    size_t fsize;
    v = malloc(sizeof(struct VFS));

    v->FP=fopen(vfsname, "r+b");
    if (!v->FP)
    {
        closeWithoutSaving(v);
        perror("[ERROR] Blad przy probie otwarcia pliku.\n");
        exit(1);
    }
    fseek(v->FP, 0, SEEK_END);
    fsize = ftell(v->FP);

    fseek(v->FP, 0, SEEK_SET);
    if (fread(&v->SB, sizeof(struct superblock), 1, v->FP) <= 0)
    {
        perror("[ERROR] Blad przy wczytaniu informacji o dysku.\n");
        closeWithoutSaving(v);
        exit(1);
    }
    v->inode_num = get_number_inodes(v->SB.size);
    v->inode_list = malloc(sizeof(struct inode) * v->inode_num);
    /*Reading inodes*/
    if (fread(v->inode_list, sizeof(struct inode), v->inode_num, v->FP) <= 0)
    {
        perror("[ERROR] przy wczytywaniu INODE.\n");
        closeWithoutSaving(v);
        exit(1);
    }
    //printf("Pomyslnie otworzono VFS\n");
    return v;
}
/*=====================================================================*/
/*=====================================================================*/
void addFileToVFS(struct VFS *v, const char* fname)
{
    FILE *new_file;
    size_t new_file_size;
    char buffer[BLOCK];
    unsigned int required_nodes, i, j;
    unsigned int *free_nodes_list;

    for (i = 0; i < v->inode_num; i++)
    {
        if(v->inode_list[i].flag == FIRST)
        {
            if (strncmp(fname, v->inode_list[i].name, FILENAME)==0)
            {
                perror("[ERROR] Istnieje juz plik o takiej nazwie.");
                closeWithoutSaving(v);
                exit(1);
            }
        }
    }

    new_file = fopen(fname, "rb");
    if (!new_file)
    {
        errno = 1;
        perror("[ERROR] Blad przy probie otwarcia pliku\n");
        closeWithoutSaving(v);
        exit(1);
    }
    fseek(new_file, 0, SEEK_END);
    new_file_size = ftell(new_file);
    if (v->SB.free_size < new_file_size)
    {
        errno = ENOMEM;
        perror("[ERROR] Brak wolnej pamieci na dysku");
        closeWithoutSaving(v);
        exit(1);
    }
    required_nodes = getReqInodes(new_file_size);
    free_nodes_list = malloc(required_nodes*sizeof(unsigned int));
    for (i = 0, j = 0; i < v->inode_num, j < required_nodes; i++)
    {
        if (v->inode_list[i].flag == FREE)
        {
            free_nodes_list[j] = i;
            j++;
            if (j == required_nodes) break;
        }
    }
    if (j != required_nodes)
    {
        perror("[ERROR] Nie znaleziono odpowiedniej ilosci wolnych blokow.\n");
        free(free_nodes_list);
        closeWithoutSaving(v);
        fclose(new_file);
        exit(3);
    }
    fseek(new_file, 0, SEEK_SET);
    for (i = 0; i < required_nodes; i++)
    {
        /*FLAG*/
        if (i == 0)
            v->inode_list[free_nodes_list[i]].flag = FIRST;
        else
            v->inode_list[free_nodes_list[i]].flag = USED;

        /*Name*/
        strncpy(v->inode_list[free_nodes_list[i]].name, fname, FILENAME);

        /*File Size*/
        if (i != required_nodes - 1)
            v->inode_list[free_nodes_list[i]].size = BLOCK;
        else
        {
            if (!( new_file_size % BLOCK))
                v->inode_list[free_nodes_list[i]].size = BLOCK;
            else
                v->inode_list[free_nodes_list[i]].size = new_file_size % BLOCK;
        }
        /*Next INODE*/
        if (i < required_nodes - 1)
            v->inode_list[free_nodes_list[i]].next_inode = free_nodes_list[i + 1];
        else
            v->inode_list[free_nodes_list[i]].next_inode = -1;

        /*Copy from file to virtual disk*/
        fseek(v->FP, getBlockOffset(free_nodes_list[i], v->inode_num), SEEK_SET);
        fread(buffer, 1, sizeof(buffer), new_file);
        fwrite(buffer, 1, v->inode_list[free_nodes_list[i]].size, v->FP);
    }
    v->SB.file_number +=1;
    v->SB.free_size -= required_nodes*BLOCK;
    free(free_nodes_list);
    fclose(new_file);
    closeAndSaveVFS(v);
    printf("Pomyslnie przekopiowano plik %s na dysk VFS.\n", fname);
}
/*=====================================================================*/
/*=====================================================================*/
void copyFileFromVFS(struct VFS* v, const char* fname, const char* newname)
{
    FILE * new_file;
    char buffer[BLOCK];
    unsigned int i, first_node;
    int found = 0;


    for (i = 0; i < v->inode_num; i++)
    {
        if (v->inode_list[i].flag == FIRST && strncmp(v->inode_list[i].name, fname, FILENAME) == 0)
        {
            first_node = i;
            found = 1;
            break;
        }
    }
    if (found==0)
    {
        perror("[ERROR] Nie znaleziono pliku na dysku VFS.\n");
        closeWithoutSaving(v);
        exit(1);
    }
    new_file = fopen(newname, "wb");
    if (!new_file)
    {
        perror("[ERROR] Blad przy probie otworzenia pliku do zapisu.\n");
        closeWithoutSaving(v);
        exit(1);
    }
    while (1)
    {
        fseek(v->FP, getBlockOffset(first_node, v->inode_num), SEEK_SET);
        fread(buffer, 1, v->inode_list[first_node].size, v->FP);
        fwrite(buffer, 1, v->inode_list[first_node].size, new_file);
        if (v->inode_list[first_node].next_inode == -1) break;
        first_node = v->inode_list[first_node].next_inode;
    }
    fclose(new_file);
    closeWithoutSaving(v);
    printf("Pomyslnie przekopiowano plik %s z dysku VFS\n.", fname);
}
/*=====================================================================*/
/*=====================================================================*/
void removeFileFromVFS(struct VFS* v, const char* fname)
{
    int current_inode, i, blocks;
    current_inode = -1;
    for (i = 0; i < v->inode_num; i++)
    {
        if (v->inode_list[i].flag == FIRST)
        {
            if (strncmp(v->inode_list[i].name, fname, FILENAME) == 0)
            {
                current_inode = i;
                break;
            }
        }
    }
    if (current_inode == -1)
    {
        perror("[ERROR] Nie znaleziono pliku na dysku VFS\n");
        closeWithoutSaving(v);
        exit(1);
    }

    blocks = 0;
    while (1)
    {
        v->inode_list[current_inode].flag = FREE;
        blocks++;
        if (v->inode_list[current_inode].next_inode != -1)
        {
            current_inode = v->inode_list[current_inode].next_inode;
        }
        else break;
    }
    v->SB.free_size += blocks*BLOCK;
    v->SB.file_number -= 1;
    closeAndSaveVFS(v);
    printf("Pomyslnie usunieto plik %s z dysku VFS\n",fname);
}
/*=====================================================================*/
/*=====================================================================*/
void destroyVFS()
{
    unlink(vfsname);
}
/*=====================================================================*/
/*=====================================================================*/
unsigned int get_number_inodes(size_t dsize)
{
    return (dsize - sizeof(struct superblock)) / (sizeof(struct inode) + BLOCK);
}
/*=====================================================================*/
/*=====================================================================*/
unsigned int getReqInodes(size_t size)
{
    size_t size_left = size;
    unsigned int req = 0;

    while (1)
    {
        req++;
        if (size_left <= BLOCK) break;
        if (size_left > BLOCK) size_left =size_left- BLOCK;
    }
    return req;
}

unsigned int getBlockOffset(unsigned int num_inode, unsigned int inode_num)
{
    return sizeof(struct superblock) + sizeof(struct inode) * inode_num + BLOCK* num_inode;
}
/*=====================================================================*/
/*=====================================================================*/
void ls(struct VFS* v)
{
    int i = 0, j,k;
    int n_node = -1;
    size_t size = 0;
    int *which_blocks = malloc(sizeof(int)* v->inode_num);
    printf("=================================================================\n");
    printf("%-32s%-12s%s\n","File Name","Size","Used Blocks");
    for (i = 0; i < v->inode_num; i++)
    {
        if (v->inode_list[i].flag == FIRST)
        {
            printf("%-32s", v->inode_list[i].name);
            j = 0;
            which_blocks[j] = i;
            n_node = v->inode_list[i].next_inode;
            size = size + v->inode_list[i].size;
            while (n_node != -1)
            {
                j++;
                which_blocks[j] = n_node;
                size = size + v->inode_list[n_node].size;
                n_node = v->inode_list[n_node].next_inode;
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
/*=====================================================================*/
/*=====================================================================*/
void diskStatistics(struct VFS* v)
{
    size_t memory_wasted = 0;
    int i = 0,j,k;
    int n_inodes=v->inode_num;
    printf("=================================================================\n");
    printf("*VFS Data:\n");
    printf("Size                  : %zu \n", v->SB.size);
    printf("Data Size             : %zu\n", sizeof(struct superblock) + sizeof(struct inode)*v->inode_num);
    printf("Block Size            : %d\n", BLOCK);
    printf("Free                  : %zu \n", v->SB.free_size);
    printf("Files                 : %d \n", v->SB.file_number);
    for (i = 0; i < v->inode_num; i++)
    {
        if (v->inode_list[i].flag == FIRST)
        {
            j = v->inode_list[i].next_inode;
            k = i;
            while (j != -1)
            {
                k = j;
                j = v->inode_list[j].next_inode;
            }
            memory_wasted = memory_wasted + (BLOCK - v->inode_list[k].size);
        }
    }
    printf("External Fragmentation: %zu\n", v->SB.size - (v->inode_num*(BLOCK+sizeof(struct inode)) + sizeof(struct superblock)) );
    printf("Internal Fragmentation: %zu\n", memory_wasted);
    printf("Block Usage:\n");
    printf("E - Empty | F - First & In Use | U - In use\n");
    for (i = 0; i < v->inode_num; i++)
    {
        printf("%3d:", i);
        if (v->inode_list[i].flag == FIRST) printf("F | ");
        else if (v->inode_list[i].flag == USED) printf("U | ");
        else printf("E | ");
        if ((i+1)%15==0 && i+1<v->inode_num) printf("\n");
    }
    printf("\n");
    printf("=================================================================\n");
}

