#pragma once

#include "Build.h"
class InsertEdge : public Build {
public:
  vector<vector<Label>> new_add_label;
  vector<vector<Label>> label_last_step, label_cur_step;

  void insertUpdateLabel();
  void weightChangePropagate();
  bool nextRoundLabelPropagate();
  vector<int> new_add_max_rank;

  vector<char> is_node_affected;
  void removeDuplicateLabelAndSort();
  void removeRedundantNewLabel();
  void removeRedundantOldLabel();
  void mergeLabel();

public:
  string updateFile;
  vector<pair<pair<int, int>, int>> updateData; // format: ID1  ID2  insert_edge_weight

  void readAndUpdateGraph() {
    ifstream inFile(updateFile);
    if (!inFile) {
      cout << "Fail to open file " << updateFile << endl;
      exit(1);
    } else {
      string line;
      while (getline(inFile, line)) {
        stringstream ss(line);
        int ID1, ID2, newWeight;
        ss >> ID1 >> ID2 >> newWeight;
        updateData.emplace_back(make_pair(ID1, ID2), newWeight);
      }

    
      cout << "updateData size: " << updateData.size() << endl;

      // update graph
      for (auto &update : updateData) {
        int ID1 = update.first.first;
        int ID2 = update.first.second;
        int newWeight = update.second;
        Neighbor[ID1].emplace_back(ID2, newWeight);
        Neighbor[ID2].emplace_back(ID1, newWeight);
      }
    }
  }
};

class InsertNode : public InsertEdge {
public:
  void readAndUpdateGraph() {
    ifstream inFile(updateFile);
    if (!inFile) {
      cout << "Fail to open file " << updateFile << endl;
      exit(1);
    } else {
      string line;
      int maxNodeID = -1;
      while (getline(inFile, line)) {
        stringstream ss(line);
        int ID1, ID2, newWeight;
        ss >> ID1 >> ID2 >> newWeight;
        maxNodeID = max(maxNodeID, max(ID1, ID2));
        updateData.emplace_back(make_pair(ID1, ID2), newWeight);
      }

      // 1.  update graph
      const int old_nodenum = nodenum;
      nodenum = maxNodeID + 1;
      edgenum += updateData.size() * 2;

      Neighbor.resize(nodenum);
      for (auto &update : updateData) {
        int ID1 = update.first.first;
        int ID2 = update.first.second;
        int newWeight = update.second;
        Neighbor[ID1].emplace_back(ID2, newWeight);
        Neighbor[ID2].emplace_back(ID1, newWeight);
      }

      // 2. update node order, new add node'order will be smallest
      NodeOrder.resize(nodenum);
      vNodeOrder.resize(nodenum);
      const int new_add_num = nodenum - old_nodenum;
      for (int i = old_nodenum - 1; i >= 0; i--) {
        vNodeOrder[i + new_add_num] = vNodeOrder[i];
      }

      vector<pair<int, int>> Deg; // pair<id, degree>
      Deg.reserve(new_add_num);

      for (int i = 0; i < new_add_num; i++) {
        int degree = Neighbor[i + old_nodenum].size();
        Deg.emplace_back(i + old_nodenum, degree);
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
        vNodeOrder[rank] = id;
        ++rank;
      }
      for (int i = 0; i < nodenum; i++) {
        NodeOrder[vNodeOrder[i]] = i;
      }

      // 3. add new empty label
      label.resize(nodenum);
      for (int i = old_nodenum; i < nodenum; i++) {
        label[i].emplace_back(i, 0, 0);
      }
      cout << "new nodenum=" << nodenum << " new edgenum=" << edgenum << endl;
    }
  }
};