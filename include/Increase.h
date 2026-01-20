#pragma once

#include "Build.h"
class Increase : public Build {

public:

  // increase: the increase  algorithms need the step information
  vector<vector<vector<int>>> step_start_pos; //__step_start_pos[nodeID][step]={}
  vector<vector<Label>> new_add_label;
  vector<vector<Label>> label_last_step, label_cur_step;

  vector<vector<pair<int, int>>> weight_unupdate;

  void increaseUpdateLabel();
  void weightChangeErrorLabel();
  bool firstRoundLabelPropagate(); 
  bool nextRoundLabelPropagate();  

  // v2
  vector<char> is_node_affected;
  vector<int> new_delete_max_rank;
  vector<vector<pair<int, int>>> weight_update;

  void increaseUpdateLabel_v2();

  void weightChangeErrorLabel_v2(); // only add: weight_update

  void nextStepErrorLabelDelete(int step);
  void errorLabelDelete();

  bool firstRoundLabelPropagate_v2(); 
  bool nextRoundLabelPropagate_v2();

  void removeDuplicateNewLabelAndSort();
  void removeRedundantNewLabel();

  void mergeLabel();

public:
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
            updateData_oldWeight[make_pair(ID1, ID2)] = Neighbor[ID1][i].second;
            Neighbor[ID1][i].second = newWeight;
            break;
          }
        }
        for (int i = 0; i < Neighbor[ID2].size(); i++) {
          if (Neighbor[ID2][i].first == ID1) {
            updateData_oldWeight[make_pair(ID2, ID1)] = Neighbor[ID2][i].second;
            Neighbor[ID2][i].second = newWeight;
            break;
          }
        }
      }
    }
  }
};