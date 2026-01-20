#include "../include/Delete.h"

void deleteEdgeExp(string &graphPath, string &binPath, string &updateFile, int threadnum) {

  DeleteEdge g;
  g.ReadGraph(graphPath);
  g.orderPath = graphPath + ".order";
  g.VertexOrdering();

  g.loadLabel(binPath);
  // g.printIndexSize();

  g.updateFile = updateFile;
  g.threadnum = threadnum;
  cout << "updateFile=" << g.updateFile << endl;
  cout << "threadnum=" << g.threadnum << endl;
  cout << "maxStep=" << g.maxStep << endl;

  g.readAndUpdateGraph();

  auto start_time = chrono::high_resolution_clock::now();
  g.deleteUpdateLabel_v2();
  auto end_time = chrono::high_resolution_clock::now();
  std::chrono::duration<double> Duration = end_time - start_time;
  cout << "【--【Delete】deleteUpdateLabel--】 Time elapsed: " << Duration.count() << " seconds" << endl;

  g.printIndexSize();
}

int main(int argc, char *argv[]) {

  if (argc < 4) {
    cerr << "Usage:\n";
    cerr << "  " << argv[0] << " 1. Graph Path 2.Update File 3.Thread Number:  \n";
    return 1;
  }

  string graphPath = argv[1];

  string updateFile = argv[2];
  int threadnum = stoi(argv[3]);

  string binPath = graphPath + "_label_step.bin";

  cout << "\n\n\nGraph Path: " << graphPath << endl;
  cout << "Bin Path: " << binPath << endl;
  cout << "Update File: " << updateFile << endl;
  cout << "Thread Number: " << threadnum << endl;

  if (1) {
    // check whether the updateFile has duplicate edges
    ifstream inFile(updateFile);
    if (!inFile) {
      cout << "Fail to open file " << updateFile << endl;
      exit(1);
    }
    set<pair<int, int>> edgeSet;
    string line;
    while (getline(inFile, line)) {
      stringstream ss(line);
      int ID1, ID2, newWeight;
      ss >> ID1 >> ID2 >> newWeight;
      if (edgeSet.find(make_pair(ID1, ID2)) != edgeSet.end() || edgeSet.find(make_pair(ID2, ID1)) != edgeSet.end()) {
        cout << "Duplicate edge: " << ID1 << " " << ID2 << endl;
        exit(1);
      }
      edgeSet.insert(make_pair(ID1, ID2));
      edgeSet.insert(make_pair(ID2, ID1));
    }
    cout << "No duplicate edges in " << updateFile << endl;
  }
  // TimeProfiler::flush_cpu_cache();
  deleteEdgeExp(graphPath, binPath, updateFile, threadnum);
}
