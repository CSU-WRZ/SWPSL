# Due to the large file size, please decompress it yourself.

## Input Graph

The first line of the input graph file contains **two integers**:
- the number of nodes \(|V|\)
- the number of edges \(|E|\)

Each subsequent line contains **three integers**:
- the ID of the first node  
- the ID of the second node  
- the weight of the edge connecting them  

Node IDs are **zero-based**, i.e., they start from `0`.  
Edge weights are **positive integers**.
The graph is **undirected**. Therefore, **each edge must be listed twice**, once for each direction.

---

## Update File

The update file specifies modifications to the graph for dynamic maintenance.  


### Edge Weight Update

This type of update modifies the weight of an existing edge.

Each line contains three integers:
- the ID of the first node  
- the ID of the second node  
- the **new edge weight** after the update  




### Edge / Vertex Insertion

Each line contains three integers:
- the ID of the first node  
- the ID of the second node  
- the edge weight

As defined in our paper, inserting an isolated vertex alone does not affect shortest distances and has no impact on labels. We therefore consider vertex insertion together with the insertion of its incident edges.

For vertex insertion: New vertex IDs are assigned consecutively, starting from the maximum node ID in the original graph plus one.


### Edge / Vertex Deletion

#### Edge Deletion

Each line contains three integers:
- the ID of the first node  
- the ID of the second node  
- the edge weight


#### Vertex Deletion

Each line contains one integer:
- the ID of the vertex to be deleted


