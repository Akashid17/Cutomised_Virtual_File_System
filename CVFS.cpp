#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>

#define MAXFILES 50
#define FILESIZE 1024

#define READ 4
#define WRITE 2

#define REGULAR 1
#define SPECIAL 2

struct SuperBlock
{ 
    int TotalInodes;
    int FreeInodes;
}Obj_Super;

struct inode
{
    char File_name[50];
    int Inode_number;
    int File_Size;
    int File_Type;
    int ActualFileSize;
    int Link_Count;
    int Reference_Count;
    char * Data;
    struct inode *next;
};

typedef struct inode INODE;
typedef struct inode * PINODE;
typedef struct inode ** PPINODE;

struct FileTable
{
    int ReadOffset;
    int WriteOffset;
    int Count;
    int Mode;
    PINODE iptr;
};

typedef struct FileTable FILETABLE;
typedef struct FileTable * PFILETABLE;

struct UFDT
{
    PFILETABLE ufdt[MAXFILES];
}UFDTobj;

PINODE Head = NULL;

bool CheckFile(char *name)
{
    PINODE temp = Head;

    while(temp != NULL)
    {
        if(temp->File_Type != 0)
        {
            if(strcmp(temp->File_name, name) == 0)
            {
                break;
            }
        }
        temp = temp->next;
    }

    if(temp == NULL)
    {
        return false;
    }
    else
    {
        return true;
    }
}

void CreateDILB()
{
    int i = 1;

    PINODE newn = NULL;
    PINODE temp = Head;

    while(i <= MAXFILES)
    {
        newn = (PINODE)malloc(sizeof(INODE));

        newn->Inode_number = i;
        newn->File_Size = FILESIZE;
        newn->File_Type = 0;
        newn->ActualFileSize = 0;
        newn->Link_Count = 0;
        newn->Reference_Count = 0;
        newn->Data = NULL;
        newn->next = NULL;

        if(Head == NULL)
        {
            Head = newn;
            temp = newn;
        }
        else
        {
            temp->next = newn;
            temp = temp->next;
        }

        i++;
    }
    printf("DILB created succesfully!!\n");
}

void CreateSuperBlock()
{
    Obj_Super.TotalInodes = MAXFILES;
    Obj_Super.FreeInodes = MAXFILES;

    printf("Super block created succesfully!!\n");
}

void CreateUDFT()
{
    int i = 0;

    for(i = 0; i < MAXFILES; i++)
    {
        UFDTobj.ufdt[i] = NULL;
    }
}

int CreateFile(char *name, int Permission)
{
    bool bRet = false;

    if((name == NULL)||(Permission > READ+WRITE)||(Permission < WRITE))
    {
        return -1;
    }

    bRet = CheckFile(name);
    if(bRet == true)
    {
        printf("File is already present\n");
        return -1;
    }

    if(Obj_Super.FreeInodes == 0)
    {
        printf("There is no inode to create the file\n");
        return -1;
    }

    int i = 0;
    for(i = 0; i < MAXFILES; i++)
    {
        if(UFDTobj.ufdt[i] == NULL)
        {
            break;
        }
    }

    if(i == MAXFILES)
    {
        printf("Unable to get entry in UDFT\n");
        return -1;
    }

    UFDTobj.ufdt[i] = (PFILETABLE)malloc(sizeof(FILETABLE));
    UFDTobj.ufdt[i]->ReadOffset = 0;
    UFDTobj.ufdt[i]->WriteOffset = 0;
    UFDTobj.ufdt[i]->Mode = Permission;
    UFDTobj.ufdt[i]->Count = 1;

    PINODE temp = Head;

    while(temp != NULL)
    {
        if(temp->File_Type == 0)
        {
            break;
        }

        temp = temp->next;
    }

    UFDTobj.ufdt[i]->iptr = temp;
    strcpy(UFDTobj.ufdt[i]->iptr->File_name,name);
    UFDTobj.ufdt[i]->iptr->File_Type = REGULAR;
    UFDTobj.ufdt[i]->iptr->ActualFileSize = 0;
    UFDTobj.ufdt[i]->iptr->Link_Count = 1;
    UFDTobj.ufdt[i]->iptr->Reference_Count = 1;

    UFDTobj.ufdt[i]->iptr->Data = (char*)malloc(sizeof(FILESIZE));
    Obj_Super.FreeInodes--;

    return i;
}

void DeleteFile(char *name)
{
    bool bRet = false;

    bRet = CheckFile(name);

    if(bRet == false)
    {
        printf("There is no such file\n");
        return;
    }

    int i = 0;

    for(i = 0; i < MAXFILES; i++)
    {
        if(strcmp(UFDTobj.ufdt[i]->iptr->File_name,name) == 0)
        {
            break;
        }
    }

    strcmp(UFDTobj.ufdt[i]->iptr->File_name,"");
    UFDTobj.ufdt[i]->iptr->File_Type = 0;
    UFDTobj.ufdt[i]->iptr->ActualFileSize = 0;
    UFDTobj.ufdt[i]->iptr->Link_Count = 0;
    UFDTobj.ufdt[i]->iptr->Reference_Count = 0;

    free(UFDTobj.ufdt[i]->iptr->Data);
    free(UFDTobj.ufdt[i]);

    UFDTobj.ufdt[i] = NULL;
    Obj_Super.FreeInodes++;
}

int WriteFile(int fd, char *arr, int size)
{
    if(UFDTobj.ufdt[fd] == NULL)
    {
        printf("Invalid file desriptor\n");
        return -1;
    }

    if(UFDTobj.ufdt[fd]->Mode == READ)
    {
        printf("There is no write permission\n");
        return -1;
    }

    strncpy(((UFDTobj.ufdt[fd]->iptr->Data)+(UFDTobj.ufdt[fd]->WriteOffset)),arr,size);

    UFDTobj.ufdt[fd]->WriteOffset = UFDTobj.ufdt[fd]->WriteOffset+size;
    UFDTobj.ufdt[fd]->iptr->ActualFileSize = UFDTobj.ufdt[fd]->iptr->ActualFileSize + size;

    return size;
}

int OpenFile(char *name, int Permission)
{
    PINODE temp = Head;
    bool bRet = false;

    if(name == NULL || Permission < 0)
    {
        printf("Incorrect parameter\n");
        return -1;
    }

    bRet = CheckFile(name);

    if(bRet == false)
    {
        printf("File not present\n");
        return -1;
    }

    int i = 0;

    while(i < MAXFILES)
    {
        if(strcmp(UFDTobj.ufdt[i]->iptr->File_name,name) == 0)
        {
            break;
        }

        i++;
    }

    if(UFDTobj.ufdt[i]->Mode < Permission)
    {
        printf("Permission denied\n");
        return -1;
    }


    i = 0;

    while(i < MAXFILES)
    {
        if(UFDTobj.ufdt[i] == NULL)
        {
            break;
        }
        i++;
    }

    UFDTobj.ufdt[i] = (PFILETABLE)malloc(sizeof(FILETABLE));

    UFDTobj.ufdt[i]->Count = 1;
    UFDTobj.ufdt[i]->Mode = Permission;

    if(UFDTobj.ufdt[i]->Mode == READ+WRITE)
    {
        UFDTobj.ufdt[i]->ReadOffset = 0;
        UFDTobj.ufdt[i]->WriteOffset = 0;
    }
    else if(UFDTobj.ufdt[i]->Mode == READ)
    {
        UFDTobj.ufdt[i]->ReadOffset = 0;
    }
    else if(UFDTobj.ufdt[i]->Mode == WRITE)
    {
        UFDTobj.ufdt[i]->WriteOffset = 0;
    }

    UFDTobj.ufdt[i]->iptr = temp;
    UFDTobj.ufdt[i]->iptr->Reference_Count++;

    return i;
}

int ReadFile(char *name, char *arr, int size)
{
    bool bRet = false;

    bRet = CheckFile(name);

    if(bRet == false)
    {
        printf("File not present\n");
        return -1;
    }

    int i = 0;

    while(i < MAXFILES)
    {
        if(strcmp(UFDTobj.ufdt[i]->iptr->File_name,name) == 0)
        {
            break;
        }
        i++;
    }

    if(UFDTobj.ufdt[i]->Mode != READ && UFDTobj.ufdt[i]->Mode !=(READ+WRITE))
    {
        printf("Permission denied\n");
        return -1;
    }

    if(UFDTobj.ufdt[i]->iptr->ActualFileSize == 0)
    {
        printf("File is empty\n");
        return -1;
    }

    if(UFDTobj.ufdt[i]->iptr->ActualFileSize < size)
    {
        size = UFDTobj.ufdt[i]->iptr->ActualFileSize;
    }

    strncpy(arr,(UFDTobj.ufdt[i]->iptr->Data)+(UFDTobj.ufdt[i]->ReadOffset),size);

    return i;
}

int CloseFile(char *name)
{
    bool bRet = false;

    bRet = CheckFile(name);

    if(bRet == false)
    {
        return -1;
    }

    int i = 0;

    while(i < MAXFILES)
    {
        if(strcmp(UFDTobj.ufdt[i]->iptr->File_name,name) == 0)
        {
            break;
        }

        i++;
    }

    UFDTobj.ufdt[i]->WriteOffset = 0;
    UFDTobj.ufdt[i]->ReadOffset = 0;
    UFDTobj.ufdt[i]->iptr->Reference_Count -= 1;

    return 0;

}

int StatFile(char *name)
{
    if(name == NULL)
    {
        return -1;
    }

    bool bRet = false;


    bRet = CheckFile(name);

    if(bRet == false)
    {
        return -2;
    }

    int i = 0;

    while(i < MAXFILES)
    {
        if(strcmp(UFDTobj.ufdt[i]->iptr->File_name,name) == 0)
        {
            break;
        }
        i++;
    }

    printf("File name: %s\n",UFDTobj.ufdt[i]->iptr->File_name);
    printf("Inode number: %d\n",UFDTobj.ufdt[i]->iptr->Inode_number);
    printf("File size: %dbytes\n",UFDTobj.ufdt[i]->iptr->File_Size);
    printf("Actual File Size: %dbytes\n",UFDTobj.ufdt[i]->iptr->ActualFileSize);
    printf("Link Count: %d\n",UFDTobj.ufdt[i]->iptr->Link_Count);
    printf("Reference Count: %d\n",UFDTobj.ufdt[i]->iptr->Reference_Count);

    return i;
}

void LS()
{
    PINODE temp = Head;

    while(temp != NULL)
    {
        if(temp->File_Type != 0)
        {
            printf("%s\n",temp->File_name);
        }

        temp = temp->next;
    }
}

void BackupFS()
{

}

void SetEnvoirnment()
{
    CreateDILB();
    CreateSuperBlock();
    CreateUDFT();
    printf("Envoirnment for the Virtual File System is set...\n");
}

void DisplayHelp()
{
    printf("-----------------------------------------------------\n");
    printf("open : It is used to open the existing file\n");
    printf("close : It is used to close the opened file\n");
    printf("read : It is used to read the contents of file\n");
    printf("write : It is used to write the data into file\n");
    printf("stat : It is used to display the information of file\n");
    printf("creat : It is used to create new regular file\n");
    printf("rm : It is used to delete regular file\n");
    printf("ls : It is used to display all names of files\n");
    printf("-----------------------------------------------------\n");
}

void ManPage(char *str)
{
    if(strcmp(str,"open") == 0)
    {
        printf("Desricption : It is used to open an existing file \n");
        printf("Usage : open File_name Mode\n");
    }
    else if(strcmp(str,"close") == 0)
    {
        printf("Desricption : It is used to close the existing file\n");
        printf("Usage : close File_name\n");
    }
    else if(strcmp(str,"read") == 0)
    {
        printf("Desricption : It is used to read data into buffer\n");
        printf("Usage : read File_name count_of_letters\n");
    }
    else if(strcmp(str,"write") == 0)
    {
        printf("Desricption : It is used to write data into file\n");
        printf("Usage : write File_name\n");
        printf("After the command please enter the data\n");
    }
    else if(strcmp(str,"lseek") == 0)
    {
        printf("Desricption : Used to change file offset\n");
        printf("Usage : lseek File_name Change_in_offset Start_point\n");
    }
    else if(strcmp(str,"stat") == 0)
    {
        printf("Description : Used to display information of file\n");
        printf("Usage : stat File_name\n");
    }
    else if (strcmp(str,"creat") == 0)
    {
        printf("Desricption : It is used to create new regular file\n");
        printf("Usage : creat File_name Permission\n");
    }
    else if (strcmp(str,"rm") == 0)
    {
        printf("Desricption : It is used to delete regular file\n");
        printf("Usage : rm File_name\n");
    }
    else if (strcmp(str,"ls") == 0)
    {
        printf("Desricption : It is used to list out all names of the files\n");
        printf("Usage : ls\n");
    }   
    else
    {
        printf("Man page not found\n");
    }
}

int main()
{
    char str[80];
    char command[4][80];
    int count = 0;

    printf("Customised Virtual File System\n\n");

    SetEnvoirnment();

    while(1)
    {
        fgets(str,80,stdin);
        fflush(stdin);

        count = sscanf(str,"%s %s %s %s",command[0],command[1],command[2],command[3]);

        if(count == 1)
        {
            if(strcmp(command[0],"help") == 0)
            {
                DisplayHelp();
            }
            else if(strcmp(command[0],"clear") == 0)
            {
                system("clear");
            }
            else if(strcmp(command[0],"exit") == 0)
            {
                BackupFS();
                printf("Thank you for using Customised Virtual File System\n");
                break;
            }
            else if(strcmp(command[0],"ls") == 0)
            {   
                LS();
            }
            else
            {
                printf("Command not found !!\n");
            }
        }
        else if(count == 2)
        {
            if(strcmp(command[0],"man") == 0)
            {
                ManPage(command[1]);
            }
            else if(strcmp(command[0],"rm") == 0)
            {
                DeleteFile(command[1]);
            }
            else if(strcmp(command[0],"stat") == 0)
            {
                int ret = StatFile(command[1]);

                if(ret == -1)
                {
                    printf("Incorrect parameter\n");
                }
                else if(ret == -2)
                {
                    printf("There is no such file\n");
                }
            }
            else if(strcmp(command[0],"write") == 0)
            {
                char arr[1024];
                printf("Please enter data to erite\n");
                fgets(arr,1024,stdin);

                fflush(stdin);

                int ret = WriteFile(atoi(command[1]),arr,strlen(arr)-1);
                if(ret != -1)
                {
                    printf("%d Bytes gets written succesfully in the file\n",ret);
                }
            }
            else if(strcmp(command[0],"close") == 0)
            {
                int fd = 0;
                fd = CloseFile(command[1]);

                if(fd == -1)
                {
                    printf("There is no such file\n");
                }
                else if(fd != -1)
                {
                    printf("File close succesfully\n");
                }
            }
            else
            {
                printf("Command not found !!\n");
            }
        }
        else if(count == 3)
        {
            if(strcmp(command[0],"creat") == 0)
            {
                int fd = 0;
                fd = CreateFile(command[1],atoi(command[2]));

                if(fd == -1)
                {
                    printf("Unable to create file\n");
                }
                else
                {
                    printf("File succesfully create with FD %d\n",fd);
                }
            }
            else if(strcmp(command[0],"open") == 0)
            {
                int fd = 0;
                fd = OpenFile(command[1],atoi(command[2]));

                if(fd == -1)
                {
                    printf("Unable to open file\n");
                }
                else if(fd > 0)
                {
                    printf("File opened succesfully with FD %d\n",fd);
                }
            }
            else if(strcmp(command[0],"read") == 0)
            {
                int fd = 0;
                int iCount = 0;

                char *str = (char*)malloc(sizeof(atoi(command[2]+1)));

                fd = ReadFile(command[1],str,atoi(command[2]));

                if(fd == -1)
                {
                    printf("Unable to read file\n");
                }
                else if(fd != -1)
                {
                    printf("Read Data: \n%s\n",str);
                }
            }
        }
        else
        {
            printf("Bad command or file name\n");
        }
    }
    return 0;
}