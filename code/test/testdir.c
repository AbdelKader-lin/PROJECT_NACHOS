#include "syscall.h"

int main() {
    int ret;
    SynchPutString("0. Starting directory syscall test at the root\n");
    /* 1. Create directory "newdir" */
    ret = CreateDirectory("newdir");
    if (ret == 0) {
        SynchPutString("1. CreateDirectory newdir failed\n");
        Halt();
    }
    SynchPutString("1. CreateDirectory newdir OK\n");

    SynchPutString("1. Listing directory contents of the root:\n");
    ListDirectory();

    /* 2. Change into "newdir" */
    ret = ChangeDirectory("newdir");
    if (ret == 0) {
        SynchPutString("2. ChangeDirectory to newdir failed\n");
        Halt();
    }
    SynchPutString("2. ChangeDirectory to newdir OK\n");
    SynchPutString("2. Listing directory contents of the newdir:\n");
    ListDirectory();

    /* 3. Create a subdirectory "subdir" */
    ret = CreateDirectory("subdir");
    if (ret == 0) {
        SynchPutString("3. Create subdir inside newdir failed\n");
        Halt();
    }
    SynchPutString("3. Create subdir inside newdir OK\n");

    SynchPutString("3. Listing directory contents of newdir:\n");
    ListDirectory();

    /* 4. Go back to parent directory */
    ret = ChangeDirectory("..");
    if (ret == 0) {
        SynchPutString("4. ChangeDirectory to the root failed\n");
        Halt();
    }
    SynchPutString("4. ChangeDirectory to root OK\n");

    SynchPutString("4. Listing directory contents of the root:\n");
    ListDirectory();

    /* 5. Back to newdir */
    ret = ChangeDirectory("newdir");
    if (ret == 0) {
        SynchPutString("5. ChangeDirectory to newdir failed\n");
        Halt();
    }
    SynchPutString("5. ChangeDirectory to newdir OK\n");
    SynchPutString("5. Listing directory contents of the newdir:\n");
    ListDirectory();

    /* 6. Remove subdirectory*/
    ret = RemoveDirectory("subdir");
    if (ret == 0) {
        SynchPutString("6. Remove subdir failed\n");
        Halt();
    }
    SynchPutString("6. Remove subdir OK\n");
    SynchPutString("6. Listing directory contents of newdir:\n");
    ListDirectory();

    /* 7. Go back */
    ret = ChangeDirectory("..");
    if (ret == 0) {
        SynchPutString("7. ChangeDirectory to root failed\n");
        Halt();
    }
    SynchPutString("7. ChangeDirectory to root OK\n");
    SynchPutString("7. Listing directory contents of the root:\n");
    ListDirectory();

    /* 10. Remove newdir */
    ret = RemoveDirectory("newdir");
    if (ret == 0) {
        SynchPutString("8. Remove newdir failed\n");
        Halt();
    }
    SynchPutString("8. Remove newdir OK\n");
    SynchPutString("8. Listing directory contents of the root:\n");
    ListDirectory();
    SynchPutString("9. Directory syscall test PASSED\n");
    Halt();
    return 0; /* never reached */
}