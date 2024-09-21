This repository contains implementation of the Basic Time Stamp Ordering Algorithm and the variants of Multi-version Time stamp ordering algorithms. The comprehensive comparison of the same is done in the report[https://github.com/rohit-kapoor/Comparison-of-BTO-and-versions-of-MVTO/blob/main/BTO_versions_of_mvto.pdf].


In order to execute the code. First navigate to the file where source code is present:

Compilation commands:

1)BTO:
$ g++ -std=c++17 -o bto BTO.cpp -pthread

2)MVTO:
$ g++ -std=c++17 -o mvto MVTO.cpp -pthread

3)MVTO-K:
$ g++ -std=c++17 -o mvtok MVTO-k.cpp -pthread

4)MVTO-GC
$ g++ -std=c++17 -o mvtoGC MVTO-gc.cpp -pthread

Execution commands:
1)BTO:
$ ./bto

2)MVTO:
$ ./mvto

3)MVTO-K:
$ ./mvtok

4)MVTO-GC:
$ ./mvtoGC

