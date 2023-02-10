#TODO: Add compilation instructions,finish the project lol

##Diagram V1:
![](https://github.com/golfrumors/CPP-Torrent/blob/main/img/diag-v1.png)

Important, the fastMemCpy() function as is in include/Utils.h,
and in src/Utils.cpp, relies on the use of AVX2 architecture,
the program does perform a check, however, the check is dependant on the
G++ compiler flag, "__builtin_cpu_supports("avx2")" and CMake doesn't have the
best history of listening to the specified compiler flags. So, do with that what you will.