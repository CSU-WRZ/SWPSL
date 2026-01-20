#include "Build.h"

#pragma once

#include "Build.h"
class DeleteEdge : public Build {

public:
  vector<vector<vector<int>>> step_start_pos; //__step_start_pos[nodeID][step]={}
  vector<vector<Label>> new_add_label;
  vector<vector<Label>> label_last_step, label_cur_step;

  vector<vector<pair<int, int>>> weight_unupdate;

  // v2
  vector<char> is_node_affected;
  vector<int> new_delete_max_rank;
  vector<vector<pair<int, int>>> weight_update;

  void deleteUpdateLabel_v2();

  void weightChangeErrorLabel_v2(); // only add: weight_update

  void nextStepErrorLabelDelete(int step);
  void errorLabelDelete();

  bool firstRoundLabelPropagate_v2();
  bool nextRoundLabelPropagate_v2();

  void removeDuplicateNewLabelAndSort();
  void removeRedundantNewLabel();

  void mergeLabel();

public:
  /*
    【query distance】
  */

  int query_type_2(int ID1, int ID2); // Q(L* L)
  int query_type_3(int ID1, int ID2); // Q(L,L*)
  int query_type_4(int ID1, int ID2); // Q(L*,L*)
public:
  string updateFile;
  vector<pair<pair<int, int>, int>> updateData;
  map<pair<int, int>, int> updateData_oldWeight;
  void readAndUpdateGraph() {
    ifstream inFile(updateFile);
    if (!inFile) {
      cout << "Fail to open file " << updateFile << endl;
      exit(1);
    } else {
      string line;
      while (getline(inFile, line)) {
        stringstream ss(line);
        int ID1, ID2, oldWeight;
        ss >> ID1 >> ID2 >> oldWeight;
        updateData.emplace_back(make_pair(ID1, ID2), oldWeight);
      }

      cout << "updateData size: " << updateData.size() << endl;

      // update graph
      for (auto &update : updateData) {
        int ID1 = update.first.first;
        int ID2 = update.first.second;
        int newWeight = update.second;
        for (auto it = Neighbor[ID1].begin(); it != Neighbor[ID1].end(); it++) {
          if ((*it).first == ID2) {
            updateData_oldWeight[make_pair(ID1, ID2)] = (*it).second;
            Neighbor[ID1].erase(it);
            break;
          }
        }
        for (auto it = Neighbor[ID2].begin(); it != Neighbor[ID2].end(); it++) {
          if ((*it).first == ID1) {
            updateData_oldWeight[make_pair(ID2, ID1)] = (*it).second;
            Neighbor[ID2].erase(it);
            break;
          }
        }
      }
    }
  }
};

class DeleteNode : public DeleteEdge {
public:
  void readAndUpdateGraph() {
    ifstream inFile(updateFile);
    if (!inFile) {
      cout << "Fail to open file " << updateFile << endl;
      exit(1);
    } else {
      string line;
      vector<int> delete_node_list;
      int ID1, ID2, newWeight;

      while (inFile >> ID1) {
        delete_node_list.push_back(ID1);
      }

      // 1. update graph

      for (int i = 0; i < delete_node_list.size(); i++) {
        int ID1 = delete_node_list[i];
        for (auto it = Neighbor[ID1].begin(); it != Neighbor[ID1].end(); it++) {
          int ID2 = (*it).first;
          updateData.emplace_back(make_pair(ID1, ID2), (*it).second);
          updateData_oldWeight[make_pair(ID1, ID2)] = (*it).second;
          updateData_oldWeight[make_pair(ID2, ID1)] = (*it).second;
          for (auto it2 = Neighbor[ID2].begin(); it2 != Neighbor[ID2].end(); it2++) {
            if ((*it2).first == ID1) {
              Neighbor[ID2].erase(it2);
              break;
            }
          }
        }
        Neighbor[ID1].clear();
      }

      cout << "update size: " << updateData.size() << endl;
    }
  }
};