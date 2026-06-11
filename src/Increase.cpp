
#include "../include/Increase.h"
void Increase::increaseUpdateLabel_v2() {

  step_start_pos.resize(nodenum, vector<vector<int>>(maxStep + 1));
#pragma omp parallel for num_threads(threadnum) schedule(dynamic)
  // #pragma omp parallel for num_threads(threadnum)
  for (int nodeID = 0; nodeID < nodenum; ++nodeID) {
    for (int i = 0; i < label[nodeID].size(); ++i) {
      int tmp = label[nodeID][i].step;
      step_start_pos[nodeID][tmp].push_back(i);
    }
  }

  maxStep = step_start_pos[0].size();

  new_add_label.resize(nodenum);
  label_last_step.resize(nodenum);
  label_cur_step.resize(nodenum);

  record_cand_dis.resize(threadnum, vector<int>(nodenum, INF));
  record_cand_set.resize(nodenum);
  record_cand_step.resize(threadnum, vector<int>(nodenum, INF));
  record_nodeID_label_dis.resize(threadnum, vector<int>(nodenum, INF));

  weightChangeErrorLabel_v2();

  for (int step = 2; step < maxStep; ++step) {
    nextStepErrorLabelDelete(step);
  }
  errorLabelDelete();

  // printIndexSize();

  //
  // type1: weight_update
  // type2: weight_unupdate: 2.1 2.2

  vector<vector<Label>> tmp_backup(label); // = label;
  int round = 1;
  bool flag = firstRoundLabelPropagate_v2();

  while (flag) {
    ++round;
    // cout << "begin: round=" << round << endl;
    std::swap(label_last_step, label_cur_step);
    flag = nextRoundLabelPropagate_v2();
  }
  cout << "nextRoundLabelPropagate: round=" << round << endl;
  swap(tmp_backup, label);

  removeDuplicateNewLabelAndSort();

  removeRedundantNewLabel(); // need the sorted label

  mergeLabel();
}

void Increase::weightChangeErrorLabel_v2() {
  vector<vector<int>> need_to_delete(nodenum);
  weight_unupdate.resize(nodenum);
  weight_update.resize(nodenum);
  set<pair<int, int>> weight_update_set;

#pragma omp parallel for num_threads(threadnum) schedule(dynamic)
  for (int ii = 0; ii < updateData.size(); ++ii) {
    auto &update = updateData[ii];
    int ID1 = update.first.first, ID2 = update.first.second;
    if (NodeOrder[ID1] > NodeOrder[ID2]) {
      swap(ID1, ID2);
    }
#pragma omp critical
    {
      weight_update_set.insert(make_pair(ID1, ID2));
      weight_update_set.insert(make_pair(ID2, ID1));
    }
#ifdef DIJKSTRA
    benchmark::heap<2, int, int> pqueue(nodenum);
    pqueue.update(ID1, 0);
    vector<char> closed(nodenum, 0);
    vector<int> distance(nodenum, INF);
    distance[ID1] = 0;
    int topNodeID, topNodeDis, NNodeID, NWeigh;
    while (!pqueue.empty()) {
      pqueue.extract_min(topNodeID, topNodeDis);
      if (topNodeID == ID2) {
        break;
      }
      closed[topNodeID] = 1;
      for (auto it = Neighbor[topNodeID].begin(); it != Neighbor[topNodeID].end(); it++) {
        NNodeID = (*it).first;
        NWeigh = (*it).second + topNodeDis;
        if (!closed[NNodeID]) {
          if (distance[NNodeID] > NWeigh) {
            distance[NNodeID] = NWeigh;
            pqueue.update(NNodeID, NWeigh);
          }
        }
      }
    }

    int new_dis = distance[ID2];
    if (new_dis <= updateData_oldWeight[make_pair(ID1, ID2)]) {
      continue;
    }
#endif


#ifndef DELETE_CHCEK
    int old_dis = INF;
    vector<pair<int, int>> commonHub; // store i j
    auto &left = label[ID1], &right = label[ID2];
    for (int i = 0, j = 0; i < left.size() && j < right.size();) {
      int hub1 = left[i].hub, hub2 = right[j].hub;
      if (hub1 == hub2) {
        int tmp = left[i].dis + right[j].dis;
        if (tmp < old_dis) {
          old_dis = tmp;
          commonHub.clear();
          commonHub.emplace_back(i, j);
        } else if (tmp == old_dis) {
          commonHub.emplace_back(i, j);
        }
        ++i;
        ++j;
      } else if (hub1 < hub2) {
        ++i;
      } else {
        ++j;
      }
    }

    if (commonHub.size() == 1) {
      int i = commonHub[0].first, hub1 = left[i].hub;
#pragma omp critical
      {
        need_to_delete[ID1].push_back(i);
      }
    }

#else
    int old_dis = INF;
    auto &left = label[ID1];
    for (int i = 0; i < left.size(); ++i) {
      int hub1 = left[i].hub;
      if (hub1 == ID2&&left[i].step == 1) { //add step == 1 check
#pragma omp critical
        {
          need_to_delete[ID1].push_back(i);
        }
        break;
      }
    }
#endif    
  }

#pragma omp parallel for num_threads(threadnum) schedule(dynamic)
  for (int i = 0; i < nodenum; ++i) {
    for (int j = 0; j < Neighbor[i].size(); ++j) {
      int neighID = Neighbor[i][j].first, weight = Neighbor[i][j].second;
      if (weight_update_set.find(make_pair(i, neighID)) == weight_update_set.end()) {
        weight_unupdate[i].push_back(make_pair(neighID, weight));
      } else {
        weight_update[i].push_back(make_pair(neighID, weight));
      }
    }
    for (int j = 0; j < need_to_delete[i].size(); ++j) {
      label[i][need_to_delete[i][j]].step = UINT8_MAX; // due to the step_start_pos, we cannot directly erase the label
    }
  }
}
void Increase::nextStepErrorLabelDelete(int step) {

#pragma omp parallel for num_threads(threadnum) schedule(dynamic)
  for (int nodeID = 0; nodeID < nodenum; ++nodeID) {
    int thread_id = omp_get_thread_num();
    auto &cand_dis = record_cand_dis[thread_id];
    for (auto idx : step_start_pos[nodeID][step]) {
      int tmp_step = label[nodeID][idx].step;
      if (tmp_step == UINT8_MAX) {
        continue;
      }
      int hub = label[nodeID][idx].hub, dis = label[nodeID][idx].dis;
      cand_dis[hub] = dis;
    }

    for (int Index = 0; Index < weight_unupdate[nodeID].size(); ++Index) {
      int neighID = weight_unupdate[nodeID][Index].first, weight = weight_unupdate[nodeID][Index].second;
      for (auto idx : step_start_pos[neighID][step - 1]) {

        int tmp_step = label[neighID][idx].step;
        if (tmp_step == UINT8_MAX) {
          continue;
        }

        int hub = label[neighID][idx].hub;
        if (cand_dis[hub] != INF) {
          int dis = label[neighID][idx].dis;
          if (dis + weight == cand_dis[hub]) {
            cand_dis[hub] = INF;
          }
        }
      }
    }

    for (auto idx : step_start_pos[nodeID][step]) {
      int hub = label[nodeID][idx].hub, dis = label[nodeID][idx].dis;

      int tmp_step = label[nodeID][idx].step;
      if (tmp_step == UINT8_MAX) {
        continue;
      }

      if (cand_dis[hub] != INF) {
        cand_dis[hub] = INF;
        label[nodeID][idx].step = UINT8_MAX; // need to be marked as deleted
      }
    }
  }
}

void Increase::errorLabelDelete() {

  is_node_affected.assign(nodenum, 0);
  new_delete_max_rank.assign(nodenum, -1);
#pragma omp parallel for num_threads(threadnum) schedule(dynamic)
  for (int nodeID = 0; nodeID < nodenum; ++nodeID) {
    auto it = label[nodeID].begin();
    while (it != label[nodeID].end()) {
      if ((*it).step == UINT8_MAX) {
        is_node_affected[nodeID] = 1;
        if (NodeOrder[(*it).hub] > new_delete_max_rank[nodeID]) {
          new_delete_max_rank[nodeID] = NodeOrder[(*it).hub];
        }
        it = label[nodeID].erase(it);
      } else {
        ++it;
      }
    }
  }
}

bool Increase::firstRoundLabelPropagate_v2() {

  bool flag = false;

#pragma omp parallel for num_threads(threadnum) schedule(dynamic)
  for (int nodeID = 0; nodeID < nodenum; ++nodeID) {
    int thread_id = omp_get_thread_num();
    auto &cand_dis = record_cand_dis[thread_id];
    auto &cand_set = record_cand_set[thread_id];
    auto &cand_step = record_cand_step[thread_id];
    label_cur_step[nodeID].clear();

    // type1: weight_update
    for (int Index = 0; Index < weight_update[nodeID].size(); ++Index) {
      int neighID = weight_update[nodeID][Index].first;

      auto &last_step = label[neighID];

      if (last_step.size() > 0) {
        // 【hub】 ---dis---> 【neighID】--neighW--【nodeID】
        for (auto it = last_step.begin(); it != last_step.end(); it++) {
          int hub = (*it).hub;

          if (NodeOrder[hub] > NodeOrder[nodeID]) {
            int dis = (*it).dis, neighW = weight_update[nodeID][Index].second, tmp_step = (*it).step;
            int d = neighW + dis;
            if (cand_dis[hub] == INF) {
              cand_dis[hub] = d;
              cand_set.push_back(hub);
              cand_step[hub] = tmp_step + 1;
            } else if (cand_dis[hub] > d) {
              cand_dis[hub] = d;
              cand_step[hub] = tmp_step + 1;
            }
          }
        }
      }
    }

    // type2: weight_unupdate
    for (int Index = 0; Index < weight_unupdate[nodeID].size(); ++Index) {
      int neighID = weight_unupdate[nodeID][Index].first;
      auto &last_step = label[neighID];
      if (last_step.size() > 0) {
        // 【hub】 ---dis---> 【neighID】--neighW--【nodeID】
        for (auto it = last_step.begin(); it != last_step.end(); it++) {
          int hub = (*it).hub;

          if (!is_node_affected[hub]) { // type2.1
            if (!is_node_affected[nodeID]) {
              continue;
            }
            if (NodeOrder[hub] > new_delete_max_rank[nodeID]) { // type2.2
              continue;
            }
          }

          if (NodeOrder[hub] > NodeOrder[nodeID]) {
            int dis = (*it).dis, neighW = weight_unupdate[nodeID][Index].second, tmp_step = (*it).step;
            int d = neighW + dis;

            if (cand_dis[hub] == INF) {
              cand_dis[hub] = d;
              cand_set.push_back(hub);
              cand_step[hub] = tmp_step + 1;
            } else if (cand_dis[hub] > d) {
              cand_dis[hub] = d;
              cand_step[hub] = tmp_step + 1;
            }
          }
        }
      }
    }

    if (cand_set.size() > 0) {
      auto &nodeID_label_dis = record_nodeID_label_dis[thread_id];
      auto &left = label[nodeID];
      for (int i = 0; i < left.size(); ++i) {
        nodeID_label_dis[left[i].hub] = left[i].dis;
      }

      for (int i = 0; i < cand_set.size(); ++i) {
        int hub = cand_set[i], TempDis = INF;
        auto &right = label[hub];
        for (int j = 0; j < right.size(); ++j) {
          int hub2 = right[j].hub;
          if (nodeID_label_dis[hub2] != INF) {
            int tmp = nodeID_label_dis[hub2] + right[j].dis;
            if (tmp < TempDis) {
              TempDis = tmp;
            }
          }
        }

        if (cand_dis[hub] < TempDis) {
          flag = true;
          label_cur_step[nodeID].emplace_back(hub, cand_dis[hub], cand_step[hub]);
          new_add_label[nodeID].emplace_back(hub, cand_dis[hub], cand_step[hub]);
        }
        cand_dis[hub] = INF;
        cand_step[hub] = INF;
      }
      cand_set.clear();

      for (int i = 0; i < left.size(); i++) {
        nodeID_label_dis[left[i].hub] = INF;
      }
    }
  }

#pragma omp parallel for num_threads(threadnum) schedule(dynamic)
  for (int nodeID = 0; nodeID < nodenum; ++nodeID) {
    auto &cur_step = label_cur_step[nodeID];
    if (cur_step.size() > 0) {
      auto &cur_label = label[nodeID];
      for (auto it = cur_step.begin(); it != cur_step.end(); it++) {
        cur_label.emplace_back((*it).hub, (*it).dis, (*it).step);
      }
    }
  }

  return flag;
}

bool Increase::nextRoundLabelPropagate_v2() {

  bool flag = false;

#pragma omp parallel for num_threads(threadnum) schedule(dynamic)
  for (int nodeID = 0; nodeID < nodenum; ++nodeID) {
    int thread_id = omp_get_thread_num();
    auto &cand_dis = record_cand_dis[thread_id];
    auto &cand_set = record_cand_set[thread_id];
    auto &cand_step = record_cand_step[thread_id];
    label_cur_step[nodeID].clear();

    // type1: weight_update
    for (int Index = 0; Index < weight_update[nodeID].size(); ++Index) {
      int neighID = weight_update[nodeID][Index].first;

      auto &last_step = label_last_step[neighID];

      if (last_step.size() > 0) {
        // 【hub】 ---dis---> 【neighID】--neighW--【nodeID】
        for (auto it = last_step.begin(); it != last_step.end(); it++) {
          int hub = (*it).hub;

          if (NodeOrder[hub] > NodeOrder[nodeID]) {
            int dis = (*it).dis, neighW = weight_update[nodeID][Index].second, tmp_step = (*it).step;
            int d = neighW + dis;

            if (cand_dis[hub] == INF) {
              cand_dis[hub] = d;
              cand_set.push_back(hub);
              cand_step[hub] = tmp_step + 1;
            } else if (cand_dis[hub] > d) {
              cand_dis[hub] = d;
              cand_step[hub] = tmp_step + 1;
            }
          }
        }
      }
    }

    // type2: weight_unupdate
    for (int Index = 0; Index < weight_unupdate[nodeID].size(); ++Index) {
      int neighID = weight_unupdate[nodeID][Index].first;
      auto &last_step = label_last_step[neighID];
      if (last_step.size() > 0) {
        // 【hub】 ---dis---> 【neighID】--neighW--【nodeID】
        for (auto it = last_step.begin(); it != last_step.end(); it++) {
          int hub = (*it).hub;

          if (!is_node_affected[hub]) { // type2.1
            if (!is_node_affected[nodeID]) {
              continue;
            }
            if (NodeOrder[hub] > new_delete_max_rank[nodeID]) { // type2.2
              continue;
            }
          }

          if (NodeOrder[hub] > NodeOrder[nodeID]) {
            int dis = (*it).dis, neighW = weight_unupdate[nodeID][Index].second, tmp_step = (*it).step;
            int d = neighW + dis;

            if (cand_dis[hub] == INF) {
              cand_dis[hub] = d;
              cand_set.push_back(hub);
              cand_step[hub] = tmp_step + 1;
            } else if (cand_dis[hub] > d) {
              cand_dis[hub] = d;
              cand_step[hub] = tmp_step + 1;
            }
          }
        }
      }
    }

    if (cand_set.size() > 0) {
      auto &nodeID_label_dis = record_nodeID_label_dis[thread_id];
      auto &left = label[nodeID];
      for (int i = 0; i < left.size(); ++i) {
        nodeID_label_dis[left[i].hub] = left[i].dis;
      }

      for (int i = 0; i < cand_set.size(); ++i) {
        int hub = cand_set[i], TempDis = INF;
        auto &right = label[hub];
        for (int j = 0; j < right.size(); ++j) {
          int hub2 = right[j].hub;
          if (nodeID_label_dis[hub2] != INF) {
            int tmp = nodeID_label_dis[hub2] + right[j].dis;
            if (tmp < TempDis) {
              TempDis = tmp;
            }
          }
        }

        if (cand_dis[hub] < TempDis) {
          flag = true;
          label_cur_step[nodeID].emplace_back(hub, cand_dis[hub], cand_step[hub]);
          new_add_label[nodeID].emplace_back(hub, cand_dis[hub], cand_step[hub]);
        }
        cand_dis[hub] = INF;
        cand_step[hub] = INF;
      }
      cand_set.clear();

      for (int i = 0; i < left.size(); i++) {
        nodeID_label_dis[left[i].hub] = INF;
      }
    }
  }

#pragma omp parallel for num_threads(threadnum) schedule(dynamic)
  for (int nodeID = 0; nodeID < nodenum; ++nodeID) {
    auto &cur_step = label_cur_step[nodeID];
    if (cur_step.size() > 0) {
      auto &cur_label = label[nodeID];
      for (auto it = cur_step.begin(); it != cur_step.end(); it++) {
        cur_label.emplace_back((*it).hub, (*it).dis, (*it).step);
      }
    }
  }

  return flag;
}

void Increase::removeDuplicateNewLabelAndSort() {
  vector<vector<Label>> new_add_label2(nodenum);
#pragma omp parallel for num_threads(threadnum) schedule(dynamic)
  for (int nodeID = 0; nodeID < nodenum; ++nodeID) {
    auto &cur_label = new_add_label[nodeID];
    if (cur_label.size() != 0) {
      int thread_id = omp_get_thread_num();
      auto &cand_dis = record_cand_dis[thread_id];
      for (auto it = cur_label.begin(); it != cur_label.end(); it++) {
        int hub = (*it).hub, dis = (*it).dis;
        if (cand_dis[hub] > dis) {
          cand_dis[hub] = dis;
        }
      }

      for (auto it = cur_label.begin(); it != cur_label.end(); it++) {
        int hub = (*it).hub, dis = (*it).dis;
        if (cand_dis[hub] == dis) {
          new_add_label2[nodeID].emplace_back(hub, cand_dis[hub], (*it).step);
          cand_dis[hub] = INF;
        }
      }
      std::sort(new_add_label2[nodeID].begin(), new_add_label2[nodeID].end());
    }
  }
  swap(new_add_label, new_add_label2);
}

void Increase::removeRedundantNewLabel() {
  vector<vector<Label>> new_add_label2(nodenum);
#pragma omp parallel for num_threads(threadnum) schedule(dynamic)
  for (int nodeID = 0; nodeID < nodenum; ++nodeID) {
    auto &cur_label = new_add_label[nodeID];
    for (auto it = cur_label.begin(); it != cur_label.end(); it++) {
      int hub = (*it).hub, dis = (*it).dis, step = (*it).step;

      int d4 = query_type_4(nodeID, hub);
      if (d4 <= dis) {
        continue;
      }
      int d2 = query_type_2(nodeID, hub);
      if (d2 <= dis) {
        continue;
      }
      int d3 = query_type_3(nodeID, hub);
      if (d3 <= dis) {

        continue;
      }
      new_add_label2[nodeID].emplace_back(hub, dis, step);
    }
  }
  swap(new_add_label, new_add_label2);
}

void Increase::mergeLabel() {

#pragma omp parallel for num_threads(threadnum) schedule(dynamic)
  for (int nodeID = 0; nodeID < nodenum; ++nodeID) {
    label[nodeID].insert(label[nodeID].end(), new_add_label[nodeID].begin(), new_add_label[nodeID].end());
    std::sort(label[nodeID].begin(), label[nodeID].end());
  }
}

int Increase::query_type_2(int ID1, int ID2) {
  if (ID1 == ID2) {
    return 0;
  }
  int d = INF;
  auto &left = new_add_label[ID1], &right = label[ID2];
  for (int i = 0, j = 0; i < left.size() && j < right.size();) {
    int hub1 = left[i].hub, hub2 = right[j].hub;
    if (hub1 == hub2) {
      if (hub2 != ID2) {
        int tmp = left[i].dis + right[j].dis;
        if (tmp < d) {
          d = tmp;
        }
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

int Increase::query_type_3(int ID1, int ID2) {
  if (ID1 == ID2) {
    return 0;
  }
  int d = INF;
  auto &left = label[ID1], &right = new_add_label[ID2];

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

int Increase::query_type_4(int ID1, int ID2) {
  if (ID1 == ID2) {
    return 0;
  }
  int d = INF;
  auto &left = new_add_label[ID1], &right = new_add_label[ID2];
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

void increaseExp(string &graphPath, string &binPath, string &updateFile, int threadnum) {

  Increase g;
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
  g.increaseUpdateLabel_v2();
  auto end_time = chrono::high_resolution_clock::now();
  std::chrono::duration<double> Duration = end_time - start_time;
  cout << "【--【Increase】increaseUpdateLabel--】 Time elapsed: " << Duration.count() << " seconds" << endl;

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
  increaseExp(graphPath, binPath, updateFile, threadnum);
}
