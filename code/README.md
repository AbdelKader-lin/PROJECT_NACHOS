The function int split_tokens(char *tokens, char cmd, int max_token) in utlis.h and utils.cc, that is used to manipulate pathnames is taken from L3 Maths-Info OS lab from the year 2024/2025 :
	https://github.com/AbdelKader-lin/L3_SYS_TP_NOTE/tree/Abdel

# Format filesystem
./build/nachos-final -f

# Test 1: halt
./build/nachos-final -cp ./build/halt halt  

./build/nachos-final -x halt

# Test 2: getputchar
./build/nachos-final -cp ./build/gpc gpc  

./build/nachos-final -x gpc

# Test 3: getputint
./build/nachos-final -cp ./build/gpi gpi  

./build/nachos-final -x gpi

# Test 4: getputstring
./build/nachos-final -cp ./build/gps gps  

./build/nachos-final -x gps

# Test 5: makethreads
./build/nachos-final -cp ./build/mt mt  

./build/nachos-final -x mt

# Test 6: threadjoin
./build/nachos-final -cp ./build/tj tj  

./build/nachos-final -x tj


# Test 8: userpages0
./build/nachos-final -cp ./build/u0 u0  

./build/nachos-final -x u0

# Test 9: userpages1
./build/nachos-final -cp ./build/u1 u1  

./build/nachos-final -x u1

# Test 12: mem_test
./build/nachos-final -cp ./build/mem_test mem_test  

./build/nachos-final -x mem_test

# Test 13: matmult
./build/nachos-final -cp ./build/matmult matmult  

./build/nachos-final -x matmult

# Test 14: sort
./build/nachos-final -cp ./build/sort sort  

./build/nachos-final -x sort

# Test 15: ProdCons
./build/nachos-final -cp ./build/ProdCons ProdCons  

./build/nachos-final -x ProdCons

# Test 16: filesysTest
./build/nachos-final -cp ./build/filesysTest filesysTest  

./build/nachos-final -x filesysTest

# Test 17: testdir
./build/nachos-final -cp ./build/testdir testdir  

./build/nachos-final -x testdir

# Test 18: fork-exec
## Copy files to be executed by ForkExec
./build/nachos-final -cp ./build/u0 u0  

./build/nachos-final -cp ./build/u1 u1

./build/nachos-final -cp ./build/fe fe  

./build/nachos-final -x fe

# Test 19: shell
./build/nachos-final -cp shell shell  

./build/nachos-final -x shell

# Test 20: nettest 
## In terminal 1:
./build/nachos-final -m 0 -osend 1  

## In terminal 2:
./build/nachos-final -m 1 -orecv

## In terminal 1:
./build/nachos-final -cp f1 f1
./build/nachos-final -m 0 -ftp 1 f1

## In terminal 2:
./build/nachos-final -m 1 -ftp_recv