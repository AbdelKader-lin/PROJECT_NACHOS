#ifndef FRAMEPROVIDER_H
#define FRAMEPROVIDER_H

#include "copyright.h"
//#include "filesys.h"
//#include "translate.h"
#include "bitmap.h"
#include "synch.h"
#include "synch.h"

class FrameProvider {
    public:
        FrameProvider(int nitems);
        ~FrameProvider();          // De-allocate FrameProvider

        int GetEmptyFrame();

        // release a frame obtained via GetEmptyFrame()
        void ReleaseFrame(int frame);

        // how many frames are still available
        int NumAvailFrame(); 

        void MarkFrame(int which);  // Set the "nth" FrameProvider
        void ClearFrame(int which); // Clear the "nth" FrameProvider
        bool TestFrame(int which);  // Is the "nth" bit set?

        void PrintFrame(); // Print contents of bitmap
    
	private:
        BitMap* bitmap;
        Semaphore* sem_frames;
};

#endif // FRAMEPROVIDER_H