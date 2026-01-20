# Introduction
This is the source code for the algorithm proposed in our paper.
This document provides instructions for compiling and running our code.


#  Compile
```c++
//support multi-threading via OpenMP
g++ -std=c++17 -W -Wall -O3 -MMD  -fopenmp  src/buildMain.cpp src/Build_new.cpp -o BUILD

g++ -std=c++17 -W -Wall -O3 -MMD  -fopenmp  src/Decrease2.cpp src/Build_new.cpp -o DECREASE
g++ -std=c++17 -W -Wall -O3 -MMD  -fopenmp  src/Increase.cpp  src/Build_new.cpp -o INCREASE

g++ -std=c++17 -W -Wall -O3 -MMD  -fopenmp  src/Insert.cpp  src/InsertEdge.cpp src/Build_new.cpp -o INSERTEDGE
g++ -std=c++17 -W -Wall -O3 -MMD  -fopenmp  src/Delete.cpp  src/DeleteEdge.cpp src/Build_new.cpp -o DELETEEDGE

g++ -std=c++17 -W -Wall -O3 -MMD  -fopenmp  src/Insert.cpp  src/InsertNode.cpp src/Build_new.cpp -o INSERTNODE
g++ -std=c++17 -W -Wall -O3 -MMD  -fopenmp  src/Delete.cpp  src/DeleteNode.cpp src/Build_new.cpp -o DELETENODE

```

# Run 

### 1. Build SWPSL Index
```c++
./BUILD <GraphPath> <ThreadNum> <TopK>
// <GraphPath>: Path to the input graph
// <ThreadNum>: Number of OpenMP threads
// <TopK>: Number of landmarks used in PLL

// Example: 
./BUILD dataset/WT/WT   100 50
./BUILD dataset/WIT/WIT 100 50
```

### 2. Decrease Update
```c++
./DECREASE <GraphPath> <UpdateFile> <ThreadNum>
// <GraphPath>: Path to the input graph
// <UpdateFile>: File containing edge-weight decrease
// <ThreadNum>: Number of OpenMP threads

// Example: 
./DECREASE dataset/WT/WT    dataset/WT/WT_decrease_rand1000.edge   100 
./DECREASE dataset/WIT/WIT  dataset/WIT/WIT_decrease_rand1000.edge 100 
```
### 3. Increase Update
```c++
./INCREASE <GraphPath> <UpdateFile> <ThreadNum>
// <GraphPath>: Path to the input graph
// <UpdateFile>: File containing edge-weight increase
// <ThreadNum>: Number of OpenMP threads

// Example: 
./INCREASE dataset/WT/WT    dataset/WT/WT_increase_rand1000.edge   100  
./INCREASE dataset/WIT/WIT  dataset/WIT/WIT_increase_rand1000.edge 100  
```

### 4. Insert Edge Update
```c++
./INSERTEDGE <GraphPath> <UpdateFile> <ThreadNum>

// Example:
./INSERTEDGE dataset/WT/WT   dataset/WT/WT_insertEdge_rand1000.edge   100 
./INSERTEDGE dataset/WIT/WIT dataset/WIT/WIT_insertEdge_rand1000.edge 100 
```

### 5. Delete Edge Update
```c++
./DELETEEDGE <GraphPath> <UpdateFile> <ThreadNum>

// Example:
./DELETEEDGE dataset/WT/WT   dataset/WT/WT_deleteEdge_rand1000.edge   100 
./DELETEEDGE dataset/WIT/WIT dataset/WIT/WIT_deleteEdge_rand1000.edge 100 
```

### 6. Insert Node Update
```c++
./INSERTNODE <GraphPath> <UpdateFile> <ThreadNum>

// Example:
./INSERTNODE dataset/WT/WT   dataset/WT/WT_insertNode_rand1000.edge   100 
./INSERTNODE dataset/WIT/WIT dataset/WIT/WIT_insertNode_rand1000.edge 100 
```

### 7. Delete Node Update
```c++
./DELETENODE <GraphPath> <UpdateFile> <ThreadNum>

// Example:
./DELETENODE dataset/WT/WT   dataset/WT/WT_deleteNode_rand1000.edge   100 
./DELETENODE dataset/WIT/WIT dataset/WIT/WIT_deleteNode_rand1000.edge 100 
```

**Note:**

Running **BUILD** will generate an index file named  
`<GraphPath>_label_step.bin` (e.g., `dataset/WT/WT_label_step.bin`),  
which contains the constructed **SWPSL** index.

The **DECREASE**, **INCREASE**, **INSERTEDGE**, **DELETEEDGE**, **INSERTNODE** and **DELETENODE** programs will automatically read this binary index file to apply edge-weight updates.  
Therefore, please ensure that the index has been successfully built using  **BUILD** before executing any dynamic update experiments.



# Data Format
**Due to the large file size, please decompress dataset yourself.**
Data is stored in the **dataset** folder.
You can refer to the data-related introduction in the dataset/data_readme.md file for detailed information about the dataset.




