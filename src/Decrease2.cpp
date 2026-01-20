
#include "../include/Decrease.h"

void Decrease::decreaseUpdateLabel() {

  new_add_label.resize(nodenum);
  label_last_step.resize(nodenum);
  label_cur_step.resize(nodenum);

  record_nodeID_label_dis.resize(threadnum, vector<int>(nodenum, INF));
  record_cand_dis.resize(threadnum, vector<int>(nodenum, INF));
  record_cand_set.resize(threadnum);
  record_cand_step.resize(threadnum, vector<int>(nodenum, INF));

  weightChangePropagate();

  bool flag = true;
  int round = 0;
  while (flag) {
    ++round;
    // cout << "begin: round=" << round << endl;
    std::swap(label_last_step, label_cur_step);
    flag = nextRoundLabelPropagate();
  }

  removeDuplicateLabelAndSort();

  removeRedundantNewLabel(); // need the sorted label

  removeRedundantOldLabel();

  mergeLabel();
}

void Decrease::weightChangePropagate() {
  vector<vector<pair<int, int>>> weightChangeNode;
  weightChangeNode.resize(nodenum);
  set<int> IDSet;
  vector<char> is_update(updateData.size(), 0);
#pragma omp parallel for num_threads(threadnum) schedule(dynamic)
  for (int i = 0; i < updateData.size(); ++i) {
    auto &update = updateData[i];
    int ID1 = update.first.first, ID2 = update.first.second, newWeight = update.second;
    int old_dis = query_psl_dis(ID1, ID2);
    if (old_dis > newWeight) {
      is_update[i] = 1;
    }
  }

  int delete_count = 0;
  for (int i = 0; i < updateData.size(); ++i) {
    if (is_update[i]) {
      auto &update = updateData[i];
      int ID1 = update.first.first, ID2 = update.first.second, newWeight = update.second;
      IDSet.insert(ID1);
      IDSet.insert(ID2);
      weightChangeNode[ID1].emplace_back(ID2, newWeight);
      weightChangeNode[ID2].emplace_back(ID1, newWeight);

      delete_count++;
    }
  }

  vector<int> IDList(IDSet.begin(), IDSet.end());
#pragma omp parallel for num_threads(threadnum) schedule(dynamic)
  for (int i = 0; i < IDList.size(); i++) {
    int nodeID = IDList[i];
    if (weightChangeNode[nodeID].size() == 0) {
      continue;
    }
    int thread_id = omp_get_thread_num();
    auto &cand_dis = record_cand_dis[thread_id];
    auto &cand_set = record_cand_set[thread_id];
    auto &cand_step = record_cand_step[thread_id];
    for (int Index = 0; Index < weightChangeNode[nodeID].size(); ++Index) {
      int neighID = weightChangeNode[nodeID][Index].first;

      auto &last_step = label[neighID];
      if (last_step.size() > 0) {
        for (auto it = last_step.begin(); it != last_step.end(); it++) {
          int hub = (*it).hub;
          if (NodeOrder[hub] > NodeOrder[nodeID]) {
            int dis = (*it).dis, neighW = weightChangeNode[nodeID][Index].second;
            int d = neighW + dis;

            if (cand_dis[hub] == INF) {
              cand_dis[hub] = d;
              cand_set.push_back(hub);
              cand_step[hub] = (*it).step + 1;
            } else if (cand_dis[hub] > d) {
              cand_dis[hub] = d;
              cand_step[hub] = (*it).step + 1;
            }
          }
        }
      }
    }
    if (cand_set.size() > 0) {
      auto &nodeID_label_dis = record_nodeID_label_dis[thread_id];
      auto &left = label[nodeID]; // old
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

          new_add_label[nodeID].emplace_back(hub, cand_dis[hub], cand_step[hub]);
          label_cur_step[nodeID].emplace_back(hub, cand_dis[hub], cand_step[hub]);
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
}

bool Decrease::nextRoundLabelPropagate() {
  bool flag = false;

#pragma omp parallel for num_threads(threadnum) schedule(dynamic)
  for (int nodeID = 0; nodeID < nodenum; ++nodeID) {
    int thread_id = omp_get_thread_num();
    auto &cand_dis = record_cand_dis[thread_id];
    auto &cand_set = record_cand_set[thread_id];
    auto &cand_step = record_cand_step[thread_id];
    label_cur_step[nodeID].clear();
    for (int Index = 0; Index < Neighbor[nodeID].size(); ++Index) { // Neighbors
      int neighID = Neighbor[nodeID][Index].first;

      auto &last_step = label_last_step[neighID];
      if (last_step.size() > 0) {
        for (auto it = last_step.begin(); it != last_step.end(); it++) {
          int hub = (*it).hub;

          if (NodeOrder[hub] > NodeOrder[nodeID]) {
            int dis = (*it).dis, neighW = Neighbor[nodeID][Index].second;
            int d = neighW + dis;

            if (cand_dis[hub] == INF) {
              cand_dis[hub] = d;
              cand_set.push_back(hub);
              cand_step[hub] = (*it).step + 1;
            } else if (cand_dis[hub] > d) {
              cand_dis[hub] = d;
              cand_step[hub] = (*it).step + 1;
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
#ifdef USING_NEW_ADD_LABLE_CHECK
      for (int i = 0; i < new_add_label[nodeID].size(); ++i) {
        nodeID_label_dis[new_add_label[nodeID][i].hub] = new_add_label[nodeID][i].dis;
      }
#endif

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

          new_add_label[nodeID].emplace_back(hub, cand_dis[hub], cand_step[hub]);
          label_cur_step[nodeID].emplace_back(hub, cand_dis[hub], cand_step[hub]);
        }
        cand_dis[hub] = INF;
        cand_step[hub] = INF;
      }
      cand_set.clear();

      for (int i = 0; i < left.size(); i++) {
        nodeID_label_dis[left[i].hub] = INF;
      }
#ifdef USING_NEW_ADD_LABLE_CHECK
      for (int i = 0; i < new_add_label[nodeID].size(); ++i) {
        nodeID_label_dis[new_add_label[nodeID][i].hub] = INF;
      }
#endif
    }
  }

  return flag;
}

void Decrease::removeDuplicateLabelAndSort() {
  vector<vector<Label>> new_add_label2(nodenum);
#pragma omp parallel for num_threads(threadnum) schedule(dynamic)
  for (int nodeID = 0; nodeID < nodenum; ++nodeID) {
    auto &cur_label = new_add_label[nodeID];

    if (cur_label.size() != 0) {
      int thread_id = omp_get_thread_num();
      auto &cand_dis = record_cand_dis[thread_id];
      auto &cand_step = record_cand_step[thread_id];
      for (auto it = cur_label.begin(); it != cur_label.end(); it++) {
        int hub = (*it).hub, dis = (*it).dis;
        if (cand_dis[hub] > dis) {
          cand_dis[hub] = dis;
          cand_step[hub] = (*it).step;
        }
      }

      for (auto it = label[nodeID].begin(); it != label[nodeID].end();) {
        int hub = (*it).hub;
        if (cand_dis[hub] != INF) {
          it = label[nodeID].erase(it);
        } else {
          ++it;
        }
      }

      for (auto it = cur_label.begin(); it != cur_label.end(); it++) {
        int hub = (*it).hub, dis = (*it).dis;
        if (cand_dis[hub] == dis) {

          new_add_label2[nodeID].emplace_back(hub, dis, (*it).step);
          cand_step[hub] = INF;
          cand_dis[hub] = INF;
        }
      }
      std::sort(new_add_label2[nodeID].begin(), new_add_label2[nodeID].end());
    }
  }
  swap(new_add_label, new_add_label2);
}

void Decrease::removeRedundantNewLabel() {

  vector<vector<Label>> new_add_label2(nodenum);
  new_add_max_rank.assign(nodenum, -1);

  is_node_affected.assign(nodenum, 0);

#pragma omp parallel for num_threads(threadnum) schedule(dynamic)
  for (int nodeID = 0; nodeID < nodenum; ++nodeID) {
    int thread_id = omp_get_thread_num();
    auto &nodeID_label_dis = record_nodeID_label_dis[thread_id];
    auto &left1 = label[nodeID];
    for (int i = 0; i < left1.size(); ++i) {
      /* if (left[i].dis < nodeID_label_dis[left[i].hub]) {
        nodeID_label_dis[left[i].hub] = left[i].dis;
      } */
      nodeID_label_dis[left1[i].hub] = left1[i].dis;
    }
    auto &left2 = new_add_label[nodeID];
    for (int i = 0; i < left2.size(); ++i) {
      nodeID_label_dis[left2[i].hub] = left2[i].dis;
    }

    auto &cur_label = new_add_label[nodeID];
    for (auto it = cur_label.begin(); it != cur_label.end(); it++) {
      int hub = (*it).hub, dis = (*it).dis;

      // int d4 = query_type_4(nodeID, hub);
      int d4 = INF;
      {
        auto &right = new_add_label[hub];
        for (int j = 0; j < right.size(); ++j) {
          int hub2 = right[j].hub;
          if (nodeID_label_dis[hub2] != INF) {
            int tmp = nodeID_label_dis[hub2] + right[j].dis;
            if (tmp < d4) {
              d4 = tmp;
            }
          }
        }
      }
      if (d4 <= dis) {
        continue;
      }

      // int d2 = query_type_2(nodeID, hub);
      int d2 = INF;
      {
        auto &right = label[hub];
        for (int j = 0; j < right.size(); ++j) {
          int hub2 = right[j].hub;
          if (hub2 != hub && nodeID_label_dis[hub2] != INF) {
            int tmp = nodeID_label_dis[hub2] + right[j].dis;
            if (tmp < d2) {
              d2 = tmp;
            }
          }
        }
      }
      if (d2 <= dis) {
        continue;
      }

      // int d3 = query_type_3(nodeID, hub);
      // int d3 = INF;
      // {
      //   auto &right = new_add_label[hub];
      //   for (int j = 0; j < right.size(); ++j) {
      //     int hub2 = right[j].hub;
      //     if (nodeID_label_dis[hub2] != INF) {
      //       int tmp = nodeID_label_dis[hub2] + right[j].dis;
      //       if (tmp < d3) {
      //         d3 = tmp;
      //       }
      //     }
      //   }
      // }
      // if (d3 <= dis) {
      //   continue;
      // }

      new_add_label2[nodeID].emplace_back(hub, dis, (*it).step);
      is_node_affected[nodeID] = 1;

      if (NodeOrder[hub] > new_add_max_rank[nodeID]) {
        new_add_max_rank[nodeID] = NodeOrder[hub];
      }
    }

    for (int i = 0; i < left1.size(); ++i) {
      nodeID_label_dis[left1[i].hub] = INF;
    }
    for (int i = 0; i < left2.size(); ++i) {
      nodeID_label_dis[left2[i].hub] = INF;
    }
  }

  swap(new_add_label, new_add_label2);
}

void Decrease::removeRedundantOldLabel() {
  vector<vector<Label>> label2(nodenum);
#pragma omp parallel for num_threads(threadnum) schedule(dynamic)
  for (int nodeID = 0; nodeID < nodenum; ++nodeID) {

    int thread_id = omp_get_thread_num();
    auto &nodeID_label_dis = record_nodeID_label_dis[thread_id];
    auto &left1 = label[nodeID];
    for (int i = 0; i < left1.size(); ++i) {
      nodeID_label_dis[left1[i].hub] = left1[i].dis;
    }
    auto &left2 = new_add_label[nodeID];
    for (int i = 0; i < left2.size(); ++i) {
      nodeID_label_dis[left2[i].hub] = left2[i].dis;
    }

    auto &cur_label = label[nodeID];
    for (auto it = cur_label.begin(); it != cur_label.end(); it++) {
      int hub = (*it).hub, dis = (*it).dis;

      if (hub == nodeID) {

        label2[nodeID].emplace_back(nodeID, 0, 0);
        continue;
      }

      int type = 0;
      if (is_node_affected[hub]) {
        type += 1;
      }
      if (is_node_affected[nodeID]) {
        type += 2;
      }

      switch (type) {
      case 0: {

        label2[nodeID].emplace_back(hub, dis, (*it).step);
        break;
      }

      case 1: {
        // int d3 = query_type_3(nodeID, hub);
        int d3 = INF;
        {
          auto &right = new_add_label[hub];
          for (int j = 0; j < right.size(); ++j) {
            int hub2 = right[j].hub;
            if (nodeID_label_dis[hub2] != INF) {
              int tmp = nodeID_label_dis[hub2] + right[j].dis;
              if (tmp < d3) {
                d3 = tmp;
              }
            }
          }
        }
        if (dis < d3) {

          label2[nodeID].emplace_back(hub, dis, (*it).step);
        }
        break;
      }

      case 2: {
        if (NodeOrder[hub] >= new_add_max_rank[nodeID]) {
          label2[nodeID].emplace_back(hub, dis, (*it).step);
          break;
        }
        //
        // int d2 = query_type_2(nodeID, hub);
        int d2 = INF;
        {
          auto &right = label[hub];
          for (int j = 0; j < right.size(); ++j) {
            int hub2 = right[j].hub;
            if (hub2 != hub && nodeID_label_dis[hub2] != INF) {
              int tmp = nodeID_label_dis[hub2] + right[j].dis;
              if (tmp < d2) {
                d2 = tmp;
              }
            }
          }
        }

        if (dis < d2) {

          label2[nodeID].emplace_back(hub, dis, (*it).step);
        }
        break;
      }

      case 3: {
        // int d4 = query_type_4(nodeID, hub);
        int d4 = INF;
        {
          auto &right = new_add_label[hub];
          for (int j = 0; j < right.size(); ++j) {
            int hub2 = right[j].hub;
            if (nodeID_label_dis[hub2] != INF) {
              int tmp = nodeID_label_dis[hub2] + right[j].dis;
              if (tmp < d4) {
                d4 = tmp;
              }
            }
          }
        }
        if (d4 <= dis) {
          break;
        }

        // int d2 = query_type_2(nodeID, hub);
        int d2 = INF;
        {
          auto &right = label[hub];
          for (int j = 0; j < right.size(); ++j) {
            int hub2 = right[j].hub;
            if (hub2 != hub && nodeID_label_dis[hub2] != INF) {
              int tmp = nodeID_label_dis[hub2] + right[j].dis;
              if (tmp < d2) {
                d2 = tmp;
              }
            }
          }
        }
        if (d2 <= dis) {
          break;
        }

        // int d3 = query_type_3(nodeID, hub);
        // if (d3 <= dis) {
        //   break;
        // }

        label2[nodeID].emplace_back(hub, dis, (*it).step);
        break;
      }
      }
    }
    for (int i = 0; i < left1.size(); ++i) {
      nodeID_label_dis[left1[i].hub] = INF;
    }
    for (int i = 0; i < left2.size(); ++i) {
      nodeID_label_dis[left2[i].hub] = INF;
    }
  }

  swap(label, label2);
}

void Decrease::mergeLabel() {

#pragma omp parallel for num_threads(threadnum) schedule(dynamic)
  for (int nodeID = 0; nodeID < nodenum; ++nodeID) {
    label[nodeID].insert(label[nodeID].end(), new_add_label[nodeID].begin(), new_add_label[nodeID].end());
    std::sort(label[nodeID].begin(), label[nodeID].end());
  }
}

int Decrease::query_type_1(int ID1, int ID2) {
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

int Decrease::query_type_2(int ID1, int ID2) {
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

int Decrease::query_type_3(int ID1, int ID2) {
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

int Decrease::query_type_4(int ID1, int ID2) {
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

void decreaseExp(string &graphPath, string &binPath, string &updateFile, int threadnum) {
  Decrease g;
  g.ReadGraph(graphPath);

  g.orderPath = graphPath + ".order";
  g.VertexOrdering();

  g.loadLabel(binPath);
  // g.printIndexSize();

  g.updateFile = updateFile;
  g.threadnum = threadnum;
  cout << "updateFile=" << g.updateFile << "\t threadnum=" << g.threadnum << endl;

  g.readAndUpdateGraph();

  auto start_time = chrono::high_resolution_clock::now();
  g.decreaseUpdateLabel();
  auto end_time = chrono::high_resolution_clock::now();
  std::chrono::duration<double> Duration = end_time - start_time;
  cout << "【--【Dec】decreaseUpdateLabel--】 Time elapsed: " << Duration.count() << " seconds" << endl;

  g.printIndexSize();
}

int main(int argc, char *argv[]) {

  if (argc < 4) {
    cerr << "Usage:\n";
    cerr << "  " << argv[0] << " 1. Graph Path 2.Update File 3.Thread Number:  \n";
    return 1;
  }

  string graphPath = argv[1], updateFile = argv[2];
  int threadnum = stoi(argv[3]);

  string binPath = graphPath + "_label_step.bin";

  cout << "\n\n\nGraph Path: " << graphPath << endl;
  cout << "Bin Path: " << binPath << endl;
  cout << "Update File: " << updateFile << endl;
  cout << "Thread Number: " << threadnum << endl;

  if (1) { // check whether the updateFile has duplicate edges
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
  decreaseExp(graphPath, binPath, updateFile, threadnum);
}