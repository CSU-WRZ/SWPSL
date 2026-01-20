#pragma once

#include "std.h"

struct Label {

  int hub, dis;
  uint8_t step; // 1 byte
  Label(int _hub, int _dis, uint8_t _step) : hub(_hub), dis(_dis), step(_step) {}

  bool operator<(const Label &other) const {
    return hub < other.hub;
  }
};

struct tmp_Label {
  int hub, dis;
  tmp_Label(int hub, int dis) : hub(hub), dis(dis) {}
  bool operator<(const Label &other) const {
    return hub < other.hub;
  }
};
class Build {

public: // 【build (WPSL)index】
  int threadnum = 100;
  int topK = 0;
  vector<vector<Label>> label;
  // vector<vector<Label>> label_last_step,label_cur_step;
  vector<vector<tmp_Label>> label_last_step, label_cur_step;
  vector<vector<int>> record_nodeID_label_dis, record_cand_dis, record_cand_set, record_cand_step;
  int maxStep = 0;
  void buildIndex_Step(int topK);
  bool nextStepBuild_Step(const int step);

  void PLL_sequence_prune(int topK);
  void PLL_dijkstra_prune(int nodeID);
  vector<int> tmp_nodeID_label_dis;

  void removeRedundantLabel();

  // new
  vector<char> is_topK;
  void PLL_parallel(int topK);
  void removeRedundantLabel(int topK);

public: // 【graph, order/rank】
  int nodenum, edgenum;
  vector<vector<pair<int, int>>> Neighbor; // Neighbor[ID1].emplace_back(make_pair(ID2, weight))
  vector<int> NodeOrder, vNodeOrder;       // NodeOrder[nodeID]=order, vNodeOrder[order]=nodeID
  string orderPath;
  void ReadGraph(const string &graphname) {
    ifstream ifs;
    ifs.open(graphname, ios::in);
    if (!ifs) {
      cout << "【Error】: " << graphname + " does not exist!" << std::endl;
    } else {
      cout << "【ReadGraph】: " << graphname << endl;
    }
    ifs >> nodenum >> edgenum;
    Neighbor.assign(nodenum, vector<pair<int, int>>());

    int ID1, ID2, weight;
    while (ifs >> ID1 >> ID2 >> weight) {
      Neighbor[ID1].emplace_back(ID2, weight); // undirected edge
    }
    ifs.close();

    cout << "【Vertex number: " << nodenum << "; Edge number: " << edgenum << "】" << endl;

    /* checkGraph();*/ // Check the graph for errors
  }

  void VertexOrdering() {

    NodeOrder.assign(nodenum, -1);
    vNodeOrder.assign(nodenum, -1);

    ifstream ifs(orderPath);
    if (ifs) {
      cout << "Open file success: orderPath=" << orderPath << endl;

      int id, order;
      ifs >> id;
      while (ifs >> id >> order) {
        NodeOrder[id] = order;
        vNodeOrder[order] = id;
      }
      ifs.close();

    } else {
      cout << "【Order by degree】" << endl;

      vector<pair<int, int>> Deg; // pair<id, degree>
      Deg.reserve(nodenum);

      for (int i = 0; i < nodenum; i++) {
        int degree = Neighbor[i].size();
        Deg.emplace_back(i, degree);
      }

      sort(Deg.begin(), Deg.end(),
           [](const pair<int, int> &a, const pair<int, int> &b) {
             if (a.second != b.second)
               return a.second < b.second; // First sort by degree in ascending order
             return a.first < b.first;     // then by ID in ascending order
           });

      int rank = 0;
      for (auto &p : Deg) {
        int id = p.first;
        NodeOrder[id] = rank;
        vNodeOrder[rank] = id;
        ++rank;
      }
    }
  }
  int query_psl_dis(int ID1, int ID2) {

    if (ID1 == ID2) {
      return 0;
    }
    int d = INF;
    auto &left = label[ID1], &right = label[ID2];
    for (int i = 0, j = 0; i < left.size() && j < right.size();) {
      int hub1 = left[i].hub, hub2 = right[j].hub;
      if (hub1 == hub2) {
        int tmp = left[i].dis + right[j].dis;
        if (tmp < d) {
          d = tmp;
        }
        ++i;
        ++j;
      } else if (hub1 < hub2) {
        ++i;
      } else {
        ++j;
      }
    }
    return d;
  }

public: // 【save/load index】
  void printIndexSize() {
    long long int tmpSize = 0, tmpNum = 0;
    for (int i = 0; i < nodenum; ++i) {
      tmpNum += label[i].size();

      tmpSize += label[i].size() * 9; //*9: 4(int hub) + 4(int dis) + 1(uint8_t step) = 9 bytes
    }
    cout << "\n【printIndexSize】" << endl;
    cout << "\tTotal Lable Item: " << tmpNum << endl;
    cout << "\tAverage Label Item:" << tmpNum / (1.0 * nodenum) << endl;
    cout << "\tIndex Size (Memory):" << tmpSize / (1.0 * 1024 * 1024) << " MB\n\n";
  }
  void saveLabel(const string &filename) {
    ofstream out(filename, ios::binary);
    if (!out) {
      cerr << "Cannot open file: " << filename << endl;
      exit(1);
    }
    //   write maxStep
    out.write(reinterpret_cast<const char *>(&maxStep), sizeof(maxStep));

    //  write label  content
    size_t outer_size = label.size();
    out.write(reinterpret_cast<const char *>(&outer_size), sizeof(outer_size));

    for (const auto &vec : label) {
      size_t inner_size = vec.size();
      out.write(reinterpret_cast<const char *>(&inner_size), sizeof(inner_size));

      //  write each Label（hub, dis, step）
      for (const auto &L : vec) {
        out.write(reinterpret_cast<const char *>(&L.hub), sizeof(int));
        out.write(reinterpret_cast<const char *>(&L.dis), sizeof(int));
        out.write(reinterpret_cast<const char *>(&L.step), sizeof(uint8_t));
      }
    }
    out.close();
  }
  void loadLabel(const string &filename) {
    ifstream in(filename, ios::binary);
    if (!in) {
      cerr << "Cannot open file: " << filename << endl;
      exit(1);
    }

    //  read maxStep
    in.read(reinterpret_cast<char *>(&maxStep), sizeof(maxStep));
    // cout << "maxStep=" << maxStep << endl;

    //  read label
    size_t outer_size = 0;
    in.read(reinterpret_cast<char *>(&outer_size), sizeof(outer_size));
    label.clear();
    label.resize(outer_size);

    for (size_t i = 0; i < outer_size; ++i) {
      size_t inner_size = 0;
      in.read(reinterpret_cast<char *>(&inner_size), sizeof(inner_size));
      label[i].reserve(inner_size);

      for (size_t j = 0; j < inner_size; ++j) {
        int hub, dis, step;
        in.read(reinterpret_cast<char *>(&hub), sizeof(int));
        in.read(reinterpret_cast<char *>(&dis), sizeof(int));
        in.read(reinterpret_cast<char *>(&step), sizeof(uint8_t));

        label[i].emplace_back(hub, dis, step);
      }
    }

    in.close();
  }
};