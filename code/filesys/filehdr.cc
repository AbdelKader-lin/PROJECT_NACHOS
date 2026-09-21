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
#include "system.h"
#include "filehdr.h"

#define NumEntries divRoundDown(SectorSize, sizeof(int))

static void ReadIndirect(int sector, int *entries) {
    char* buffer = new char[SectorSize];
    memset(buffer, 0xFF, SectorSize);
    synchDisk->ReadSector(sector, buffer);
    memcpy(entries, buffer, NumEntries * sizeof(int));
    delete[] buffer;
    return;
}

static void FreeDataBlocks(BitMap *freeMap, int *blocks, int count) {
    for (int i = 0; i < count; i++) {
        ASSERT(freeMap->Test(blocks[i]));
        freeMap->Clear(blocks[i]);
    }
}

static int AllocateOne(BitMap *freeMap) {
    int s = freeMap->Find();
    ASSERT(s != -1);
    return s;
}

static void WriteIndirectBlock(int sector, int *entries, int count) {
    char* buffer = new char[SectorSize];
    memset(buffer, 0xFF, SectorSize);
    memcpy(buffer, entries, count * sizeof(int));
    synchDisk->WriteSector(sector, buffer);
    delete[] buffer;
    return;
}

static void ReadIndirectBlock(int sector, int *entries) {
    char* buffer = new char[SectorSize];
    memset(buffer, 0xFF, SectorSize);
    synchDisk->ReadSector(sector, buffer);
    memcpy(entries, buffer, NumEntries * sizeof(int));
    delete[] buffer;
    return;
}

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

bool FileHeader::Allocate(BitMap *freeMap, int fileSize) {
    numBytes   = fileSize;
    numSectors = divRoundUp(fileSize, SectorSize);

    if (numSectors > freeMap->NumClear())
        return FALSE;

    singleIndirectPointer = -1;
    doubleIndirectPointer = -1;

    unsigned int remaining = numSectors;

    /* Layer 0 */
    int directCount = remaining < NumDirect - 2 ? remaining : NumDirect - 2;
    for (int i = 0; i < directCount; i++) {
        int s = AllocateOne(freeMap);
        dataSectors[i] = s;
    }
    remaining -= directCount;

    /* Layer 1 */
    if (remaining > 0) {
        singleIndirectPointer = AllocateOne(freeMap);
        ASSERT(singleIndirectPointer != -1);

        int singleCount = remaining < NumEntries ? remaining : NumEntries;
        int entries[NumEntries];
        for (int i = 0; i < singleCount; i++) {
            int s = AllocateOne(freeMap);
            entries[i] = s;
        }

        WriteIndirectBlock(singleIndirectPointer, entries, singleCount);
        remaining -= singleCount;
    }

    /* Layer 2 */
    if (remaining > 0) {
        doubleIndirectPointer = AllocateOne(freeMap);
        ASSERT(doubleIndirectPointer != -1);

        int numLayer1 = divRoundUp(remaining, NumEntries);
        ASSERT((unsigned)numLayer1 <= NumEntries); // sanity check

        int layer1[NumEntries]; // pointers to layer2 blocks

        for (int i = 0; i < numLayer1; i++) {
            int layer2Count = remaining < NumEntries ? remaining : NumEntries;
            int layer2[NumEntries];

            // allocate data blocks for this layer2
            for (int j = 0; j < layer2Count; j++) {
                int s = AllocateOne(freeMap);
                layer2[j] = s;
            }

            // allocate sector for this layer2 block
            int layer2Sector = AllocateOne(freeMap);
            ASSERT(layer2Sector != -1);
            layer1[i] = layer2Sector;

            WriteIndirectBlock(layer2Sector, layer2, layer2Count);
            remaining -= layer2Count;
        }

        // write double indirect block
        WriteIndirectBlock(doubleIndirectPointer, layer1, numLayer1);
    }
    ASSERT(remaining == 0);
    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void FileHeader::Deallocate(BitMap *freeMap) {
    /* Layer 0 */
    int directCount = ((unsigned int)numSectors <= NumDirect - 2 ? numSectors : NumDirect - 2);
    for (int i = 0; i < directCount; i++) {
        ASSERT(freeMap->Test(dataSectors[i]));
        freeMap->Clear(dataSectors[i]);
    }

    /* Layer 1 */
    if (singleIndirectPointer != -1) {
        int entries[NumEntries];
        ReadIndirect(singleIndirectPointer, entries);

        int singleCount = numSectors - (NumDirect - 2) < NumEntries ?
                        numSectors - (NumDirect - 2) :
                        NumEntries;

        FreeDataBlocks(freeMap, entries, singleCount);

        ASSERT(freeMap->Test(singleIndirectPointer));
        freeMap->Clear(singleIndirectPointer);
        singleIndirectPointer = -1;
    }

    /* Layer 2 */
    if (doubleIndirectPointer != -1) {
        int layer1[NumEntries];
        ReadIndirect(doubleIndirectPointer, layer1);

        int remaining =
            numSectors - (NumDirect - 2) - NumEntries;

        int numLayer1 = divRoundUp(remaining, NumEntries);

        for (int i = 0; i < numLayer1; i++) {
            ASSERT(layer1[i] != -1);
            int layer2[NumEntries];
            ReadIndirect(layer1[i], layer2);

            int count = (unsigned int)remaining < NumEntries ? remaining : NumEntries;
            FreeDataBlocks(freeMap, layer2, count);

            ASSERT(freeMap->Test(layer1[i]));
            freeMap->Clear(layer1[i]);

            remaining -= count;
        }
        ASSERT(freeMap->Test(doubleIndirectPointer));
        freeMap->Clear(doubleIndirectPointer);
        doubleIndirectPointer = -1;
    }

    numBytes   = 0;
    numSectors = 0;
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void FileHeader::FetchFrom(int sector) {
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void FileHeader::WriteBack(int sector) {
    synchDisk->WriteSector(sector, (char *)this); 
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

int FileHeader::ByteToSector(int offset) {
    ASSERT(offset >= 0);
    ASSERT(offset < numBytes);

    unsigned int logical = divRoundDown(offset, SectorSize);

    /* Layer 0 */
    if (logical < NumDirect - 2) {
        ASSERT(dataSectors[logical] != -1);
        return dataSectors[logical];
    }

    logical -= (NumDirect - 2);

    /* Layer 1 */
    if (logical < NumEntries) {
        ASSERT(singleIndirectPointer != -1);

        int entries[NumEntries];
        ReadIndirect(singleIndirectPointer, entries);

        ASSERT(entries[logical] != -1);
        return entries[logical];
    }

    logical -= NumEntries;

    /* Layer 2 */
    ASSERT(doubleIndirectPointer != -1);

    int layer1[NumEntries];
    ReadIndirect(doubleIndirectPointer, layer1);

    unsigned int l1 = divRoundDown(logical, NumEntries);
    unsigned int l2 = logical % NumEntries;

    ASSERT(l1 < NumEntries);
    ASSERT(layer1[l1] != -1);

    int layer2[NumEntries];
    ReadIndirect(layer1[l1], layer2);

    ASSERT(layer2[l2] != -1);
    return layer2[l2];
}
//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int FileHeader::FileLength() {
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void FileHeader::Print() {
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    for (i = 0; i < numSectors; i++)
	    printf("%d ", dataSectors[i]);
    printf("\nFile contents:\n");
    for (i = k = 0; i < numSectors; i++) {
	    synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
	        if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
		        printf("%c", data[j]);
            else
		    printf("\\%x", (unsigned char)data[j]);
	    }
        printf("\n"); 
    }
    delete [] data;
}

int FileHeader::AddDirect(BitMap *freeMap, int numAdd) {
    int added = 0;
    int directLimit = NumDirect - 2;

    int start = numSectors < directLimit ? numSectors : directLimit;

    for (int i = start; i < directLimit && numAdd > 0; i++) {
        dataSectors[i] = AllocateOne(freeMap);
        added++;
        numAdd--;
    }

    return added;
}

int FileHeader::AddSingleIndirect(BitMap *freeMap, int maxSectors) {
    int added = 0;
    int directLimit = NumDirect - 2;
    int singleLimit = NumEntries;

    int used = numSectors > directLimit ? numSectors - directLimit : 0;
    if (used >= singleLimit) return 0; // full

    if (singleIndirectPointer == -1) {
        singleIndirectPointer = AllocateOne(freeMap);
        char zero[SectorSize];
        memset(zero, 0xFF, SectorSize);
        synchDisk->WriteSector(singleIndirectPointer, zero);
    }

    int entries[NumEntries];
    ReadIndirectBlock(singleIndirectPointer, entries);

    int canAdd = maxSectors < singleLimit - used ? maxSectors : singleLimit - used;

    for (int i = 0; i < canAdd; i++) {
        entries[used + i] = AllocateOne(freeMap);
        added++;
    }

    WriteIndirectBlock(singleIndirectPointer, entries, NumEntries);
    return added;
}

int FileHeader::AddDoubleIndirect(BitMap *freeMap, int maxSectors) {
    int added = 0;
    int directLimit = NumDirect - 2;
    int singleLimit = NumEntries;
    int doubleBase  = directLimit + singleLimit;

    if (numSectors < doubleBase) return 0;

    unsigned int logical = numSectors - doubleBase;

    if (doubleIndirectPointer == -1) {
        doubleIndirectPointer = AllocateOne(freeMap);
        char zero[SectorSize];
        memset(zero, 0xFF, SectorSize);
        synchDisk->WriteSector(doubleIndirectPointer, zero);
    }

    int l1[NumEntries];
    ReadIndirectBlock(doubleIndirectPointer, l1);

    while (maxSectors > 0 && logical < NumEntries * NumEntries) {
        unsigned int i1 = divRoundDown(logical, NumEntries);
        unsigned int i2 = logical % NumEntries;
        if (l1[i1] == -1) {
            l1[i1] = AllocateOne(freeMap);
            char zero[SectorSize];
            memset(zero, 0xFF, SectorSize);
            synchDisk->WriteSector(l1[i1], zero);
        }
        int l2[NumEntries];
        ReadIndirectBlock(l1[i1], l2);
        for (; i2 < NumEntries && maxSectors > 0; i2++) {
            l2[i2] = AllocateOne(freeMap);
            logical++;
            maxSectors--;
            added++;
        }
        WriteIndirectBlock(l1[i1], l2, NumEntries);
    }

    WriteIndirectBlock(doubleIndirectPointer, l1, NumEntries);
    return added;
}

bool FileHeader::Add(BitMap *freeMap, int additionalSize) {
    ASSERT(additionalSize > 0);

    int oldBytes   = numBytes;
    int newBytes   = numBytes + additionalSize;

    int oldSectors = divRoundUp(oldBytes, SectorSize);
    int newSectors = divRoundUp(newBytes, SectorSize);

    int toAddSectors = newSectors - oldSectors;
    int addedSectors = 0;

    if (toAddSectors > 0) {
        int directAdded = AddDirect(freeMap, toAddSectors);
        addedSectors += directAdded;
        toAddSectors -= directAdded;
    }

    if (toAddSectors > 0) {
        int singleAdded = AddSingleIndirect(freeMap, toAddSectors);
        addedSectors += singleAdded;
        toAddSectors -= singleAdded;
    }

    if (toAddSectors > 0) {
        int doubleAdded = AddDoubleIndirect(freeMap, toAddSectors);
        addedSectors += doubleAdded;
        toAddSectors -= doubleAdded;
    }

    ASSERT(toAddSectors == 0);

    numBytes   = newBytes;
    numSectors = newSectors;

    return TRUE;
}