#include "syscall.h"
int main() {
    int a, b, sum;
    const char str[] = "Enter number a: ";
    SynchPutString(str);
    SynchGetInt(&a);
    SynchPutString("The number a is: ");
    SynchPutInt(a);
    SynchPutChar('\n');
    const char str2[] = "Enter number b: \n";
    SynchPutString(str2);
    SynchGetInt(&b);
    SynchPutString("The number b is: ");
    SynchPutInt(b);
    SynchPutChar('\n');
    sum = a + b;
    SynchPutString("Sum is: ");
    SynchPutInt(sum);
    SynchPutChar('\n');   
}