// filehdr.cc
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector,
//
//      Unlike in a real system, we do not keep track of file permissions,
//	ownership, last modification date, etc., in the file header.
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "filehdr.h"
#include "debug.h"
#include "synchdisk.h"
#include "main.h"

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool FileHeader::Allocate(PersistentBitmap *freeMap, int fileSize)
{
    numBytes = fileSize;
    numSectors = divRoundUp(fileSize, SectorSize);
    if (freeMap->NumClear() < numSectors)
        return FALSE; // not enough space

    // lab12
    if (numSectors <= NumDirect)
    {
        for (int i = 0; i < numSectors; i++)
        {
            dataSectors[i] = freeMap->FindAndSet();
            // since we checked that there was enough free space,
            // we expect this to succeed
            ASSERT(dataSectors[i] >= 0);
        }
    }
    else
    {
        for (int i = 0; i < NumDirect; i++)
        {
            dataSectors[i] = freeMap->FindAndSet();
            // since we checked that there was enough free space,
            // we expect this to succeed
            ASSERT(dataSectors[i] >= 0);
        }
        dataSectors[IndirectSectorIdx] = freeMap->FindAndSet();
        int indirect_index[SectorSize / sizeof(int)];
        for (int i = 0; i < numSectors - NumDirect; i++)
        {
            indirect_index[i] = freeMap->FindAndSet();
            // since we checked that there was enough free space,
            // we expect this to succeed
            ASSERT(indirect_index[i] >= 0);
        }
        kernel->synchDisk->WriteSector(dataSectors[IndirectSectorIdx], (char *)indirect_index);
        // 仅仅适用于IndirectIndex为1的情况 ，若大于1要判断是否创建新的二级索引
    }
    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void FileHeader::Deallocate(PersistentBitmap *freeMap)
{
    if (numSectors <= NumDirect)
    {
        for (int i = 0; i < numSectors; i++)
        {
            ASSERT(freeMap->Test((int)dataSectors[i])); // ought to be marked!
            freeMap->Clear((int)dataSectors[i]);
        }
    }
    else
    {
        char *indirect_index = new char[SectorSize];
        kernel->synchDisk->ReadSector(dataSectors[IndirectSectorIdx], indirect_index);
        for (int i = 0; i < numSectors - NumDirect; i++)
        {
            freeMap->Clear((int)indirect_index[i * 4]);
        }
        for (int i = 0; i < NumDirect; i++)
        {
            freeMap->Clear((int)dataSectors[i]);
        }
    }
}
//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk.
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void FileHeader::FetchFrom(int sector)
{
    kernel->synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk.
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void FileHeader::WriteBack(int sector)
{
    kernel->synchDisk->WriteSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int FileHeader::ByteToSector(int offset)
{
    if (offset < SectorSize * NumDirect)
    {
        return dataSectors[offset / SectorSize];
    }
    else
    {
        const int sectorNum = (offset / SectorSize - NumDirect);
        int indirect_index[SectorSize / sizeof(int)];
        kernel->synchDisk->ReadSector(dataSectors[IndirectSectorIdx], (char *)indirect_index);

        return indirect_index[sectorNum];
    }
}
//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    // lab12
    printf("type:%s\n", type);
    printf("last visit time:%s", lastVisitTime);
    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    // lab12 4k
    if (numSectors <= NumDirect)
    {
        for (i = 0; i < numSectors; i++)
        {
            printf("%d ", dataSectors[i]);
        }
    }
    else
    {
        printf("direct_index:\n");
        for (i = 0; i < NumDirect; i++)
        {
            printf("%d ", dataSectors[i]);
        }
        printf("\nindirect_index:(mapping table sector %d)\n", dataSectors[IndirectSectorIdx]);
        char *indirect_index = new char[SectorSize];
        kernel->synchDisk->ReadSector(dataSectors[IndirectSectorIdx], indirect_index);
        j = 0;
        for (i = 0; i < numSectors - NumDirect; i++)
        {
            printf("%d ", int(indirect_index[j]));
            j = j + 4;
        }
    }

    printf("\nFile contents:\n");
    if (numSectors <= NumDirect)
    {
        for (i = k = 0; i < numSectors; i++)
        {
            kernel->synchDisk->ReadSector(dataSectors[i], data);
            for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
            {
                if ('\040' <= data[j] && data[j] <= '\176')
                { // isprint(data[j1)
                    printf("%c", data[j]);
                }
                else
                {
                    printf("\\%x", (unsigned char)data[j]);
                }
            }
            printf("\n");
        }
    }
    else
    {
        for (i = k = 0; i < NumDirect; i++)
        {
            printf("sector: %d\n", dataSectors[i]);
            kernel->synchDisk->ReadSector(dataSectors[i], data);
            for (j = 0; (j < SectorSize) & (k < numBytes); j++, k++)
            {
                if ('\040' <= data[j] && data[j] <= '\176') // isprint(data[il)
                    printf("%c", data[j]);
                else
                    printf("\\%x", (unsigned char)data[j]);
            }
            printf("\n");
        }
        char *indirect_index = new char[SectorSize];
        kernel->synchDisk->ReadSector(dataSectors[NumDirect], indirect_index);
        for (i = 0; i < numSectors - NumDirect; i++)
        {
            printf("sector: %d\n", int(indirect_index[i * 4]));
            kernel->synchDisk->ReadSector(int(indirect_index[i * 4]), data);
            for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
            {
                if ('\040' <= data[j] && data[j] <= '\176') // isprint(data[il)
                    printf("%c", data[j]);
                else
                    printf("\\%x", (unsigned char)data[j]);
            }
            printf("\n");
        }
    }
    printf("----------------------------------------------\n");
    delete[] data;
}

void FileHeader::setLastVisitTime()
{
    time_t timep;
    time(&timep);
    strncpy(lastVisitTime, asctime(gmtime(&timep)), 25);
    lastVisitTime[26]='\0';
}
