#include "frameprovider.h"
#include "copyright.h"
#include "machine.h"
#include "machine.h"
#include "system.h"

#include <strings.h> 


FrameProvider::FrameProvider(int nitems) {
    bitmap = new BitMap(nitems);
    sem_frames = new Semaphore("Iteration_sem", 1);
}

FrameProvider::~FrameProvider() {
    delete bitmap;
    delete sem_frames;
}

int FrameProvider::GetEmptyFrame() {
    sem_frames->P();
    int res = bitmap->Find();
    sem_frames->V();

    bzero(&(machine->mainMemory[res * PageSize]), PageSize);
    return res;
}

void FrameProvider::ReleaseFrame(int idx_frame) {
    sem_frames->P();
    bitmap->Clear(idx_frame);
    sem_frames->V();
}

/* how many frames are still available */
int FrameProvider::NumAvailFrame() {
    sem_frames->P();
    int res = bitmap->NumClear();
    sem_frames->V();
    return res;
}

void FrameProvider::MarkFrame(int which) {  // Set the "nth" FrameProvider
    bitmap->Mark(which);
}

void FrameProvider::ClearFrame(int which) { // Clear the "nth" FrameProvider
    bitmap->Clear(which);
} 

bool FrameProvider::TestFrame(int which) { // Is the "nth" bit set?
    return bitmap->Test(which);
} 

void FrameProvider::PrintFrame() { // Print contents of bitmap
    bitmap->Print();
} 