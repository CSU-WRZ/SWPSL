#pragma once

#include "Build.h"
class Decrease : public Build {
public:
  // decrease
  vector<vector<Label>> new_add_label;
  vector<vector<Label>> label_last_step, label_cur_step;

  void decreaseUpdateLabel();
  void weightChangePropagate();

  bool nextRoundLabelPropagate();
  vector<int> new_add_max_rank;
  vector<char> is_node_affected;
  void removeDuplicateLabelAndSort();
  void removeRedundantNewLabel();
  void removeRedundantOldLabel();
  void mergeLabel();

public:
  /*
    【query distance】
  */
  // query types for update
  int query_type_1(int ID1, int ID2); // Q(L,L)
  int query_type_2(int ID1, int ID2); // Q(L* L)
  int query_type_3(int ID1, int ID2); // Q(L,L*)
  int query_type_4(int ID1, int ID2); // Q(L*,L*)

public:
  string updateFile;
  vector<pair<pair<int, int>, int>> updateData; // format: ID1  ID2  newWeight
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
        for (int i = 0; i < Neighbor[ID1].size(); i++) {
          if (Neighbor[ID1][i].first == ID2) {
            Neighbor[ID1][i].second = newWeight;
            break;
          }
        }
        for (int i = 0; i < Neighbor[ID2].size(); i++) {
          if (Neighbor[ID2][i].first == ID1) {
            Neighbor[ID2][i].second = newWeight;
            break;
          }
        }
      }
    }
  }
};