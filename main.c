
//- tworzenie wirtualnego dysku,
//
//- kopiowanie pliku z dysku systemu Minix na dysk wirtualny,
//
//- kopiowanie pliku z dysku wirtualnego na dysk systemu Minix,
//
//- wyświetlanie katalogu dysku wirtualnego,
//
//- usuwanie pliku z wirtualnego dysku,
//
//- usuwanie wirtualnego dysku,
//
//- wyświetlenie zestawienia z aktualną mapą zajętości wirtualnego dysku -
//czyli listy kolejnych obszarów wirtualnego dysku z opisem: adres, typ
//        obszaru, rozmiar, stan (np. dla bloków danych: wolny/zajęty).


#include <stdlib.h>
#include <stdio.h>
#include "FileSystem.h"
/*Wyswietlenie listy komend*/
void help(char *command)
{
    printf("\n============================================================\n");
    printf("Uzycie:%s <komenda> ", command);
    printf("Lista komend: \n");
    printf("c [rozmiar w bajtach]- Utworzenie wirtualnego dysku\n");
    printf("m [nazwa_pliku]      - przeniesienie pliku na wirtualny dysk\n");
    printf("v [nazwa_pliku]      - przeniesienie pliku z wirtualnego dysku\n");
    printf("l - wyswietlenie listy plikow\n");
    printf("s - wyswietlenie statystyki dysku\n");
    printf("d - usuniecie dysku\n");
    printf("r [nazwa_pliku] - usuniecie zadanego pliku\n");
    printf("============================================================\n");
}

int main(int argc, char * argv[])
{
    struct FileSystem* v;
    if (argc<2) help(argv[0]);
    else {
        switch (argv[1][0])
        {
            case 'c':
            {
                v = createFileSystem(atoi(argv[2]));

                break;
            }
            case 'm':
            {
                v = openFileSystem();
                copyFileToFileSystem(v, argv[2]);
                break;
            }
            case 'v':
            {
                if (argc>2)
                {
                    v = openFileSystem();
                    copyFileFromFileSystem(v, argv[2], argv[3]);
                }
                else printf("Brak nazwy pliku.\n");
                break;
            }
            case 'l':
            {
                v = openFileSystem();
                listFiles(v);
                break;
            }
            case 's':
            {
                v = openFileSystem();
                fileSystemStatistics(v);
                break;
            }
            case 'd':
            {
                deleteFileSystem();
                break;
            }
            case 'r':
            {
                v = openFileSystem();
                deleteFileFromFileSystem(v, argv[2]);
                break;
            }
            default:
            {
                help(argv[0]);
                break;
            }

        }
    }
    return 0;
}