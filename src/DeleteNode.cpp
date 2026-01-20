#include "../include/Delete.h"

void deleteNodeExp(string &graphPath, string &binPath, string &updateFile, int threadnum) {

  DeleteNode g;
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



  // TimeProfiler::flush_cpu_cache();
  deleteNodeExp(graphPath, binPath, updateFile, threadnum);
}
