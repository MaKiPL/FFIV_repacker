#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
//#include <Windows.h>
#include <direct.h>

#include "Compressions.h"

#define debug 0

typedef int errno_t;

char * GetFilename(char* path);
char * GetDirectoryname(char * path);
char * GetFilenameWithoutExtension(char*path);
char * GetExtension(char*path);

int mysize;
void ParseCommands(int argc, char *argv[]);

int Decompress(char * path);
int Repack(char*path);
void ShowHelp();

int _unlz77(char*infile, char*outfile);
int lz77(char*infile, char*outfile);

int main(int argc, char *argv[])
{
    if(debug)
    {
        char * path = "E:\\files\\BTL_CAMERA.dat";
        Decompress(path);
        return 0;
    }
    ParseCommands(argc, argv);
    return 0;
}

int Decompress(char * path)
{
	FILE * f = fopen(path, "rb");
	if (f == 0)
	{
	    printf("%s", path);
		perror(" -");
		return -1;
	}
	char* magic = (char*)malloc(sizeof(char) * 4);
	fseek(f, 0, SEEK_SET);
	int res = fread(magic, sizeof(char), 4, f);
	if (strncmp(magic, "SSAM", 4) != 0)
	{
		printf("This is not FFIV container!");
		fclose(f);
		free(magic);
		return -1;
	}

	unsigned int count = 0;
	fread(&count, sizeof(int), 1, f);

	int * pointers = (int*)malloc(count * sizeof(int));
	int * sizes = (int*)malloc(count * sizeof(int));
	//char names[count+sizeof(char)*32];// = (char*)malloc(count*sizeof(char)*32);
	char * names = (char*)malloc(count * sizeof(char) * 32);// = (char*)malloc(count*sizeof(char)*32);


	unsigned int i;
	for (i = 0; i<count; i++)
	{
		fread(&pointers[i], sizeof(int), 1, f);
		fread(&sizes[i], sizeof(int), 1, f);
		fread(&names[i * 32], sizeof(char), 32, f);
	}

	unsigned int relativeFiles = count * 40 + 8;



	char* OutputDir = (char*)malloc(256);
	sprintf(OutputDir,"%s%s%s", GetDirectoryname(path), GetFilenameWithoutExtension(path), "dec\\");
	printf("%s", OutputDir);
		_mkdir(OutputDir);


	for (i = 0; i<count; i++)
	{
		fseek(f, relativeFiles + pointers[i], SEEK_SET);
		//char buffer[*(sizes + i) * sizeof(char)];// = (char*)malloc(sizes[i] * sizeof(char));
		unsigned char *buffer = (unsigned char*)malloc(sizes[i] * sizeof(char));
		res = fread(buffer, sizeof(unsigned char), sizes[i], f);
        if( *(buffer) == 0x10)
        {
            //unsigned char * retbuff = UNLZ77(buffer, sizes[i]);
            buffer = UNLZ77(buffer, sizes[i]);
            sizes[i] = mysize;
        }
		char * temppath = (char*)malloc(256);
		sprintf(temppath, "%s%s", OutputDir, (names + i * 32));
		FILE * ff = fopen(temppath, "rb");
		int bb = (int)ff;
		if(ff != 0)
			fclose(ff);
		if (bb != 0)
			remove(temppath);
		ff = fopen(temppath, "w+b");
		if (ff == 0)
		{
			perror("ERROR: ");
			return 1;
		}
		res = fwrite(buffer, sizeof(char), sizes[i], ff);
		if(!res)
            perror("\t\tError saving: ");
		printf("\nWriting:\t%s\t%ud/%ud  (%.2f%%)", (names+i*32), i + 1, count, (float)((float)(i+1)/(float)count)*100.0f);
		fclose(ff);
		free(temppath);
	}

	fclose(f);
	free(magic);
	free(pointers);
	free(sizes);
	return 0;
}

int Repack(char*path)
{
	/*WIN32_FIND_DATA DIR;
	char * windowspath = calloc(256, 1);
	strcat(windowspath, path);
	strcat(windowspath, "\\*.*");

	int fileCount = 0;

	HANDLE hat = FindFirstFile(windowspath, &DIR);

	if (hat == INVALID_HANDLE_VALUE) return 1;
	fileCount++;
	while (FindNextFile(hat, &DIR))
		fileCount++;


	char * filenames = calloc(sizeof(char) * fileCount * 64, 1);

	hat = FindFirstFile(windowspath, &DIR);
	strcpy(filenames, DIR.cFileName);
	int i=1;
	for (; i < fileCount; i++)
	{
		FindNextFile(hat, &DIR);
		strcpy((filenames + i * 64), DIR.cFileName);
	}*/

    DIR * dir;
    struct dirent * ent;
    dir = opendir(path);
    if(dir == 0)
    {
        printf("No such directory?");
        return -1;
    }
    int fileCount = 0;
    while((ent = readdir(dir)) != 0)
        fileCount++;

    rewinddir(dir);

    char * filenames = malloc(sizeof(char) * fileCount * 64);
    memset(filenames, 0x00, fileCount*64);
    int filesindex=0;
    while((ent= readdir(dir))!=0)
        strcpy(filenames+(64*filesindex++), ent->d_name);
    closedir(dir);

    int i = 0;

    int CorrectDisplay = 0;
    if(*filenames == '.')
        {
            i=2;
            CorrectDisplay = 1;
        }
    /*for(; i<fileCount; i++)
        printf("\n%s", (filenames)+64*i);*/
    char * NewPath = calloc(256,1);
    char * fn = GetFilename(path);
    if(strlen(fn) == 0)
    {
        printf("Please provide path without \\ at the end!");
        exit(1);
    }
    char * fnnew = calloc(256,1);

    strncpy(fnnew, fn, strlen(fn)-3);
    //*(fnnew+(strlen(fn)-3)) = 0x00;
    sprintf(NewPath, "%s%s%s", GetDirectoryname(path),fnnew, ".datTEST");

	FILE * ff = fopen(NewPath, "rb");
    int bb = (int)ff;
	if(ff != 0)
		fclose(ff);
	if (bb != 0)
		remove(NewPath);

	FILE * f= fopen(NewPath, "a+b");

    fwrite("SSAM", sizeof(char), 4, f);
    fileCount = CorrectDisplay? fileCount-2 : fileCount;
    fwrite(&fileCount, 4, 1, f);
    fileCount = CorrectDisplay? fileCount+2 : fileCount;

    int* RealOffsets=malloc(sizeof(int)*fileCount*4);
    int* RealSizes=malloc(sizeof(int)*fileCount*4);
    char * absolutepath = (char*)malloc(256);
    char * uncompbuff = (char*)malloc(0xFFFFFF);

    int uncompsize;
    int res;

    for(; i<fileCount; i++)
    {
        memset(absolutepath, 0x00, 255);
        strcat(absolutepath, path);
        if(*(path+strlen(path)-1) != '\\')
            strcat(absolutepath, "\\");
        strcat(absolutepath, filenames+i*64);
		FILE * frr = fopen(absolutepath, "rb");
		if (frr == 0)
		{
			perror("ERROR: ");
			exit(1);
		}
        fseek(frr, 0, SEEK_END);
        uncompsize = ftell(frr);
        rewind(frr);
        //unsigned char * uncompbuff = malloc(uncompsize*sizeof(unsigned char*));
        memset(uncompbuff, 0x00, 0xFFFFFF);
        res = fread(uncompbuff, 1, uncompsize, frr);
        if(res == 0)
        {
            perror("No bytes read. Error: ");
            exit(1);
        }
        res = fclose(frr);


        if(CorrectDisplay)
        {
            if(i==2)
                {*RealOffsets = 0; *RealSizes = 0;}
            else
                *(RealOffsets+(i-2)*4) = *(RealOffsets+(i-3)*4) + 8  +  *(RealSizes +(i-3)*4);
        }
        else
        {
            if(i==0)
                {*RealOffsets = 0; *RealSizes = 0;}
            else
                *(RealOffsets+i*4) = *(RealOffsets+(i-1)*4) + 8 +  *(RealSizes +(i-1)*4);
        }

        remove(absolutepath);
		FILE * frrr = fopen(absolutepath, "w+b");
        if(frrr==0)
        {
            perror("Error...");
            printf("\n\nFilename: %s", absolutepath);
            exit(1);
        }
        unsigned char * mybuff = LZ77(uncompbuff, uncompsize);
        res = fwrite(mybuff, sizeof(char), mysize, frrr);
        free(mybuff);
        if(res == 0)
        {
            perror("Error saving compressed buffer: ");
            exit(1);
        }
        fclose(frrr);

        if(CorrectDisplay)
            *(RealSizes+(i-2)*4) = mysize;
        else
            *(RealSizes+i*4) = mysize;
        if(!CorrectDisplay)
            printf("\n%s\tSize\t%d\t%d/%d\t(%.2f%%)", filenames+i*64, mysize, i+1, fileCount, (float)((float)(i+1)/(float)fileCount)*100.0f);
        else
            printf("\n%s\tSize\t%d\t%d/%d\t(%.2f%%)", filenames+i*64, mysize, i-1, fileCount-2, (float)((float)(i-1)/(float)(fileCount-2))*100.0f);

        if(CorrectDisplay)
        {
        fwrite(RealOffsets + (i-2)*4, 4,1,f);
        fwrite(RealSizes +(i-2)*4, 4,1,f);
        fwrite(filenames+i*64, sizeof(char), 32, f);
        }
        else
        {
        fwrite(&RealOffsets + i*4, 4,1,f);
        fwrite(&RealSizes +i*4, 4,1,f);
        fwrite(filenames+i*64, sizeof(char), 32, f);
        }

        //free(uncompbuff);
    }

    for(i=0;i<fileCount-2; i++)
    {
        *absolutepath = 0x00;
        strcat(absolutepath, path);
        if(*(path+strlen(path)-1) != '\\')
            strcat(absolutepath, "\\");
        strcat(absolutepath, filenames+(i+2)*64);

		FILE * frr = fopen(absolutepath, "rb");
        fseek(frr, 0, SEEK_END);
        int uncompsize = ftell(frr);
        unsigned char * uncompbuff = (unsigned char*)malloc(uncompsize);
        rewind(frr);
        int res = fread(uncompbuff, sizeof(char), uncompsize, frr);
        if(res == 0)
        {
            perror("No bytes read. Error: ");
            exit(0);
        }
        fclose(frr);
        fwrite(uncompbuff, sizeof(char), uncompsize, f);
        fwrite("\0\0\0\0\0\0\0\0", sizeof(char), 8, f);
		free(uncompbuff);
    }


    fclose(f);
	return 0;
}


char * GetDirectoryname(char* path)
{
	char * p = strrchr(path, '\\');
	if(!p) return "";
	size_t buffsize = p - path + 1;
	char * c = (char*)calloc(buffsize,1);

	strncpy(c,path, buffsize);
	*(c+buffsize) = 0x00;
	return c;
}

char * GetFilename(char* path)
{
    /*if(*(path + strlen(path) -1) == '\\')
        return strrchr(path+strlen(path)-2, '\\') + 1;*/
    if(!strrchr(path, '\\')) return path;
	return strrchr(path, '\\') + 1;
}

char * GetFilenameWithoutExtension(char*path)
{
	char * pp = GetFilename(path);
	char * p = strrchr(pp, '.');
	char * c = calloc(p - pp,1);
	strncpy(c, pp, p - pp);
	*(c+(p-pp)) = 0x00;
	return c;
}

char * GetExtension(char*path)
{
    if(!strrchr(path, '.')) return path;
    return strrchr(path, '.')+1;
}

void ParseCommands(int argc, char * argv[])
{
    if(argc <= 1)
        ShowHelp();

    if(argc == 2)
    {
        printf("Insufficient arguments");
        return;
    }
    if(!strcmp(argv[1], "-e"))
    {
        Decompress(argv[2]);
        return;
    }

    if(!strcmp(argv[1], "-r"))
    {
        Repack(argv[2]);
        return;
    }

    if(!strcmp(argv[1], "-unlz77"))
    {
        if(argc!=4)
        {
            printf("INSUFFICIENT arguments. Probably missing output filename");
            return;
        }
        _unlz77(argv[2], argv[3]);
        return;
    }

    if(!strcmp(argv[1], "-lz77"))
    {
        lz77(argv[2], argv[3]);
        return;
    }

    return;
}

int lz77(char*infile, char*outfile)
{
FILE * fout = fopen(outfile, "w+b");
FILE * fin = fopen(infile, "rb");
if(fout == 0 || fin == 0)
{
    perror("Error opening files to read and write: ");
    return 1;
}
fseek(fin, 0, SEEK_END);
int buffsize = ftell(fin);
rewind(fin);
unsigned char * buffer = malloc(sizeof(char) * buffsize);
int res = fread(buffer, sizeof(char), buffsize, fin);
if(res == 0)
{
    perror("No bytes read! Error: ");
    return 1;
}
fclose(fin);
char * lzsed = LZ77(buffer, buffsize);
fwrite(lzsed, sizeof(char), mysize, fout);
fclose(fout);
printf("File compressed");
return 0;
}

int _unlz77(char*infile, char*outfile)
{
	FILE * ff;
	FILE * freader = fopen(infile, "rb");
    if(freader == 0)
    {
        perror("ERROR WITH FILE: ");
        return -1;
    }
    fseek(freader, 0, SEEK_END);
    int filesize = ftell(freader);
    rewind(freader);
    unsigned char * buffer = (unsigned char*)malloc( filesize*sizeof(unsigned char) );
    size_t res = fread(buffer, sizeof(unsigned char), filesize, freader);
	if (res == 0)
	{
		perror("Error: ");
		return 1;
	}
    if(*buffer !=0x10)
    {
        printf("Not LZ77 compressed file; Header is wrong!");
		fclose(freader);
		free(buffer);
        return -1;
    }
    buffer = UNLZ77(buffer, filesize);
    ff = fopen(outfile, "rb");
    int bb = (int)ff;
	if(ff != 0)
        fclose(ff);
	if (bb != 0)
		remove(outfile);
	ff =fopen(outfile, "w+b");
	if(ff == 0)
    {
        perror("Output file error: ");
		fclose(freader);
        return -1;
    }
	res = fwrite(buffer,sizeof(char), mysize, ff);
	if(res==0)
	{
	    perror("Nothing wrote. Last message: ");
		fclose(ff);
		fclose(freader);
	    return -1;
	}
	fclose(ff);
	fclose(freader);
	printf("File unpacked succesfully!");
	return 0;
}


void ShowHelp()
{
    printf("Welcome to Final Fantasy IV-Steam tool");
    printf("\n\n\nUsage:\nFFIVSteamTool.exe [Option] [File]\n\tOptions:\n");
    printf("-e (infile) - Extract\n\n-r (folderName) - Repack files in folder (must be ending with dec)\n\n");
    printf("-unlz77 (infile) (outfile) - Decompress infile using LZ77 algorithm to outfile\n\n-lz77 (infile) (outfile) - Compress infile using LZ77 and output to outfile\n\n");
    exit(0);
}
