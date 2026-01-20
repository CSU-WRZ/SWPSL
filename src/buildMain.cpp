#include "../include/Build.h"
void buildExp(string &graphPath, int threadnum, int topK) {

  Build g;
  g.threadnum = threadnum;
  g.ReadGraph(graphPath);

  g.orderPath = graphPath + ".order";
  g.VertexOrdering();

  auto start_time = chrono::high_resolution_clock::now();
  g.buildIndex_Step(topK); // optimize by PLL
  auto end_time = chrono::high_resolution_clock::now();
  std::chrono::duration<double> Duration = end_time - start_time;
  cout << "【--【build】buildIndex_Step--】 Time elapsed: " << Duration.count() << " seconds" << endl;

  string filename = graphPath + "_label_step.bin";

  g.saveLabel(graphPath + "_label_step.bin");

  cout << "【Save label to: " << filename << "】" << endl;

  g.printIndexSize();
}

int main(int argc, char *argv[]) {

  if (argc != 4) {
    cerr << "Usage:\n";
    cerr << "  " << argv[0] << " 1.Graph Path 2.Thread Number 3.topK\n";
    return 1;
  }

  string graphPath = argv[1];
  int threadnum = stoi(argv[2]), topK = stoi(argv[3]);

  cout << "\n\n\nGraph Path: " << graphPath << endl;
  cout << "Thread Number: " << threadnum << endl;
  cout << "TopK: " << topK << endl;

  // TimeProfiler::flush_cpu_cache();
  buildExp(graphPath, threadnum, topK);
}
