## Comparison BTO and Variants of MVTO

This repository contains implementation of the Basic Time Stamp Ordering Algorithm and the variants of Multi-version Time stamp ordering algorithms.
The comprehensive comparison of the same is done in the [report]( https://github.com/rohit-kapoor/Comparison-of-BTO-and-versions-of-MVTO/blob/main/BTO_versions_of_mvto.pdf ). 

In Case 1 (MVTO) we assume that there are an infinite number of versions avaiable for each data item hence
a read operstion will never fail beacuse there will always be a version of the data item lower that the time-stamp of the given transaction to read from.
In case 2 ( MVTO-k), we maintain at max k versions for the data item, hence a transaction with a time stamp lower than the time stamp of the oldest version 
will abort. For case 3 ( MVTO-GC), garbage aollection is performed when a version will no longer be useful.

#### Execution

In order to execute the code. First navigate to the file where source code is present:

Compilation commands:

1)BTO:

``` g++ -std=c++17 -o bto BTO.cpp -pthread```

2)MVTO:

``` g++ -std=c++17 -o mvto MVTO.cpp -pthread```

3)MVTO-K:

```g++ -std=c++17 -o mvtok MVTO-k.cpp -pthread```

4)MVTO-GC:

``` g++ -std=c++17 -o mvtoGC MVTO-gc.cpp -pthread```

Execution commands:
1)BTO:

```./bto```

2)MVTO:

```./mvto```

3)MVTO-K:

```./mvtok```

4)MVTO-GC:

```./mvtoGC```

