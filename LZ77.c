#include <stdio.h>
#include <stdlib.h>

#include "Compressions.h"

int localindex = 0;
int outbuffindex = 0;

unsigned char ReadByte(unsigned char * buffer);
unsigned short ReadUInt16(unsigned char * buffer);
void WriteByte(unsigned char * buffer, unsigned char _value);
void WriteBytes(unsigned char * buffer, unsigned char * _value, int offset,int length);
unsigned char * buff;


unsigned char * LZ77 (char*buffer, int buffsize)
{
    if(buffsize > 0xFFFFFF)
    {
        printf("Error, too big input file!");
        exit(1);
    }

    return _LZ77((unsigned char*)buffer, buffsize);

    /*int compressedLength = 4;

    unsigned char * outbuff = (unsigned char*)malloc(8*2+1);
    *(outbuff) = 0x00;
    int bufferlength = 1, bufferedblocks = 0;
    int readBytes = 0;
    while(readBytes < buffsize)
    {
        if(bufferedblocks==8)
        {
            writebytes(outbuffer, outbuff, 0, bufferlength);
            compressedLength += bufferlength;
            *outbuff = 0;
            bufferlength = 1;
            bufferedblocks = 0;
        }
    }
*/
}


unsigned char * UNLZ77(unsigned char*buffer, int buffsize)
{
    localindex = 1;
    int size = ReadUInt16(buffer) | (ReadByte(buffer) << 16);
    unsigned char * outbuff = (unsigned char*)malloc(size);

    int i = 0;
    int j = 0;
    while(outbuffindex < size)
    {
        int flagByte = ReadByte(buffer);
        for(i = 0; i<8; i++)
        {
            if((flagByte & (0x80 >> i)) == 0)
            {
                *(outbuff+outbuffindex++) = ReadByte(buffer);
            }
            else
                {
                    unsigned short block = ReadUInt16(buffer);
                    int count = ((block>>4) & 0xF) + 3;
                    int disp = ((block & 0xF) << 8) | ((block >> 8)&0xFF);
                    long outPos = outbuffindex;
                    long copyPos = outbuffindex - disp - 1;

                    for (j = 0; j<count; j++)
                    {
                        outbuffindex = copyPos++;
                        unsigned char b = *(outbuff+outbuffindex++);
                        outbuffindex = outPos++;
                        *(outbuff+outbuffindex++) = b;
                    }
                }
                if(outbuffindex >= size)
                    break;
        }
    }
    outbuffindex = 0;
    mysize = size;
    return outbuff;
}

unsigned char ReadByte(unsigned char * buffer)
{
    localindex++;
    return (unsigned char)*(buffer+localindex-1);
}

unsigned short ReadUInt16(unsigned char * buffer)
{
    unsigned char byte1 = ReadByte(buffer);
    unsigned char byte2= ReadByte(buffer);
    return (byte2 << 8) | byte1;
}

void WriteByte(unsigned char * buffer, unsigned char _value)
{
    *(buffer + localindex++) = _value;
}

void WriteBytes(unsigned char * buffer, unsigned char * _value, int offset,int length)
{
int i;
for(i = 0; i<length; i++)
    *(buffer+offset++) = *(_value+i);
}
