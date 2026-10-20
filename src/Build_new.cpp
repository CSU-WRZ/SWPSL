
#include "../include/Build.h"

void Build::buildIndex_Step(int topK) {

  label.resize(nodenum);
  label_last_step.resize(nodenum);
  label_cur_step.resize(nodenum);

  record_nodeID_label_dis.resize(threadnum, vector<int>(nodenum, INF));
  record_cand_dis.resize(threadnum, vector<int>(nodenum, INF));
  record_cand_set.resize(threadnum);

  for (int i = 0; i < threadnum; ++i) {
    record_cand_set[i].reserve(nodenum);
  }

  maxStep = 0;

  if (topK != 0) {
    is_topK.resize(nodenum, 0);

    PLL_parallel(topK);
  }

#pragma omp parallel for num_threads(threadnum)
  for (int i = 0; i < nodenum; ++i) {

    label[i].emplace_back(i, 0, 0);
    label_cur_step[i].emplace_back(i, 0);
  }

  removeRedundantLabel();

  int step = 0;
  bool flag = true;
  while (flag) {
    ++step;
    std::swap(label_last_step, label_cur_step);
    flag = nextStepBuild_Step(step);
  }

  if (step - 1 > maxStep) {
    maxStep = step - 1;
  }
  cout << "【maxStep=" << maxStep << "】" << endl;


  if (topK != 0) {
    removeRedundantLabel(topK);
  } else {
    removeRedundantLabel();
  }

#pragma omp parallel for num_threads(threadnum) schedule(dynamic)
  for (int nodeID = 0; nodeID < nodenum; ++nodeID) {
    std::sort(label[nodeID].begin(), label[nodeID].end());
  };
}

bool Build::nextStepBuild_Step(const int step) {
  bool flag = false;

#pragma omp parallel for num_threads(threadnum) schedule(dynamic)
  //   #pragma omp parallel for num_threads(threadnum)
  for (int nodeID = 0; nodeID < nodenum; ++nodeID) {
    int thread_id = omp_get_thread_num();
    auto &cand_dis = record_cand_dis[thread_id];
    auto &cand_set = record_cand_set[thread_id];

    for (int Index = 0; Index < Neighbor[nodeID].size(); ++Index) { // Neighbors
      int neighID = Neighbor[nodeID][Index].first;
      auto &last_step = label_last_step[neighID];
      if (last_step.size() > 0) {
        // 【hub】 ---dis---> 【neighID】--neighW--【nodeID】
        for (auto it = last_step.begin(); it != last_step.end(); it++) {
          int hub = (*it).hub;

          if (NodeOrder[hub] > NodeOrder[nodeID]) {
            int dis = (*it).dis, neighW = Neighbor[nodeID][Index].second;
            int d = neighW + dis;
            if (cand_dis[hub] == INF) {
              cand_dis[hub] = d;
              cand_set.push_back(hub);
            } else if (cand_dis[hub] > d) {
              cand_dis[hub] = d;
            }
          }
        }
      }
    }
    if (cand_set.size() > 0) {

      auto &nodeID_label_dis = record_nodeID_label_dis[thread_id];
      auto &cur_step = label_cur_step[nodeID];
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
          cur_step.emplace_back(hub, cand_dis[hub]);

          flag = true;
        }
        cand_dis[hub] = INF;
      }
      cand_set.clear();

      for (int i = 0; i < left.size(); i++) {
        nodeID_label_dis[left[i].hub] = INF;
      }
    }
  }
  if (flag) {

#pragma omp parallel for num_threads(threadnum) schedule(dynamic)
    for (int nodeID = 0; nodeID < nodenum; ++nodeID) {
      auto &cur_step = label_cur_step[nodeID];
      if (cur_step.size() > 0) {
        /* flag = true;*/
        for (auto it = cur_step.begin(); it != cur_step.end(); it++) {
          auto &cur_label = label[nodeID];
          int hub = (*it).hub;

          int i_flag = 0;
          ////i_flag=cur_label.size();
          for (; i_flag < cur_label.size(); ++i_flag) {
            if (cur_label[i_flag].hub == hub) {
              cur_label[i_flag].dis = (*it).dis;
              cur_label[i_flag].step = step; // here
              break;
            }
          }
          if (i_flag == cur_label.size()) {

            cur_label.emplace_back(hub, (*it).dis, step);
          }
        }
      }

      label_last_step[nodeID].clear();
    }
  }

  return flag;
}

void Build::PLL_dijkstra_prune(int nodeID) {
  auto &left = label[nodeID];
  for (int i = 0; i < left.size(); ++i) {
    tmp_nodeID_label_dis[left[i].hub] = left[i].dis;
  }

  benchmark::heap<2, int, int> pqueue(nodenum);
  pqueue.update(nodeID, 0);
  vector<char> closed(nodenum, 0);
  vector<int> distance(nodenum, INF);
  vector<int> step(nodenum, INF);

  distance[nodeID] = 0;
  step[nodeID] = 0;

  int topNodeID, topNodeDis, NNodeID, NWeigh;
  pqueue.extract_min(topNodeID, topNodeDis);
  closed[topNodeID] = 1;
  /* label[topNodeID].emplace_back(nodeID, topNodeDis); */

  for (auto it = Neighbor[topNodeID].begin(); it != Neighbor[topNodeID].end(); it++) {
    NNodeID = (*it).first;
    NWeigh = (*it).second + topNodeDis;
    if (NodeOrder[nodeID] > NodeOrder[NNodeID] && !closed[NNodeID]) {
      // if (!closed[NNodeID]) {
      if (NodeOrder[nodeID] > NodeOrder[NNodeID] && !closed[NNodeID]) {
        if (distance[NNodeID] > NWeigh) {
          distance[NNodeID] = NWeigh;
          step[NNodeID] = step[topNodeID] + 1;
          pqueue.update(NNodeID, NWeigh);
        }
      }
    }
  }

  while (!pqueue.empty()) {
    pqueue.extract_min(topNodeID, topNodeDis);
    closed[topNodeID] = 1;

    int tempDis = INF;

    auto &right = label[topNodeID];
    for (int j = 0; j < right.size(); ++j) {
      int hub2 = right[j].hub;
      if (tmp_nodeID_label_dis[hub2] != INF) {
        int tmp = tmp_nodeID_label_dis[hub2] + right[j].dis;
        if (tmp < tempDis) {
          tempDis = tmp;
        }
      }
    }

    if (topNodeDis < tempDis) {

      label[topNodeID].emplace_back(nodeID, topNodeDis, step[topNodeID]);
      if (step[topNodeID] > maxStep) {
        maxStep = step[topNodeID];
      }
    } else {
      continue;
    }

    for (auto it = Neighbor[topNodeID].begin(); it != Neighbor[topNodeID].end(); it++) {
      NNodeID = (*it).first;
      NWeigh = (*it).second + topNodeDis;
      // if (!closed[NNodeID]) {
      if (NodeOrder[nodeID] > NodeOrder[NNodeID] && !closed[NNodeID]) {
        if (distance[NNodeID] > NWeigh) {
          distance[NNodeID] = NWeigh;
          step[NNodeID] = step[topNodeID] + 1;
          pqueue.update(NNodeID, NWeigh);
        }
      }
    }
  }

  for (int i = 0; i < left.size(); i++) {
    tmp_nodeID_label_dis[left[i].hub] = INF;
  }
}

void Build::PLL_parallel(int topK) {
  struct tmp_triple {
    int nodeID, dis, step;
    tmp_triple(int _nodeID, int _dis, int _step) : nodeID(_nodeID), dis(_dis), step(_step) {}
  };
  vector<vector<tmp_triple>> dis_PLL(topK);

// #pragma omp parallel for num_threads(threadnum)
#pragma omp parallel for num_threads(threadnum) schedule(dynamic)
  for (int i = nodenum - 1; i >= nodenum - topK; --i) {
    int nodeID = vNodeOrder[i];
    is_topK[nodeID] = 1;
    // cout << "  PLL nodeID: " + to_string(nodeID) << endl;

    auto &dis_tid = dis_PLL[nodenum - 1 - i];

    benchmark::heap<2, int, int> pqueue(nodenum);
    pqueue.update(nodeID, 0);
    vector<char> closed(nodenum, 0);
    vector<int> step(nodenum, INF);
    vector<int> distance(nodenum, INF);

    distance[nodeID] = 0;
    step[nodeID] = 0;

    int topNodeID, topNodeDis, NNodeID, NWeigh;
    pqueue.extract_min(topNodeID, topNodeDis);
    closed[topNodeID] = 1;

    for (auto it = Neighbor[topNodeID].begin(); it != Neighbor[topNodeID].end(); it++) {
      NNodeID = (*it).first;
      NWeigh = (*it).second + topNodeDis;
      if (NodeOrder[nodeID] > NodeOrder[NNodeID] && !closed[NNodeID]) {
        if (distance[NNodeID] > NWeigh) {
          distance[NNodeID] = NWeigh;
          step[NNodeID] = step[topNodeID] + 1;
          pqueue.update(NNodeID, NWeigh);
        }
      }
    }

    while (!pqueue.empty()) {
      pqueue.extract_min(topNodeID, topNodeDis);
      closed[topNodeID] = 1;

      dis_tid.emplace_back(topNodeID, topNodeDis, step[topNodeID]);

      for (auto it = Neighbor[topNodeID].begin(); it != Neighbor[topNodeID].end(); it++) {
        NNodeID = (*it).first;
        NWeigh = (*it).second + topNodeDis;
        if (NodeOrder[nodeID] > NodeOrder[NNodeID] && !closed[NNodeID]) {
          if (distance[NNodeID] > NWeigh) {
            distance[NNodeID] = NWeigh;
            step[NNodeID] = step[topNodeID] + 1;
            pqueue.update(NNodeID, NWeigh);
          }
        }
      }
    }
  }

  for (int i = 0; i < topK; ++i) {
    int nodeID = vNodeOrder[nodenum - 1 - i];
    auto &dis_tid = dis_PLL[i];
    for (int j = 0; j < dis_tid.size(); ++j) {
      int topNodeID = dis_tid[j].nodeID, dis = dis_tid[j].dis, step = dis_tid[j].step;
      /*  if (step > maxStep) {
         maxStep = step;
       } */
      label[topNodeID].emplace_back(nodeID, dis, step);
    }
  }
}

void Build::removeRedundantLabel() {

  vector<vector<Label>> new_label_vec(nodenum);

  record_nodeID_label_dis.clear();
  record_nodeID_label_dis.resize(threadnum, vector<int>(nodenum, INF));
// #pragma omp parallel for num_threads(threadnum)
#pragma omp parallel for num_threads(threadnum) schedule(dynamic)
  for (int ID1 = 0; ID1 < nodenum; ID1++) {
    int thread_id = omp_get_thread_num();

    auto &new_label = new_label_vec[ID1];

    auto &nodeID_label_dis = record_nodeID_label_dis[thread_id];
    auto &left = label[ID1];
    for (int i = 0; i < left.size(); ++i) {
      if (left[i].dis < nodeID_label_dis[left[i].hub]) {
        nodeID_label_dis[left[i].hub] = left[i].dis;
      }
      // nodeID_label_dis[left[i].hub] = left[i].dis;
    }

    for (int j = 0; j < label[ID1].size(); j++) {

      int ID2 = label[ID1][j].hub, d1 = label[ID1][j].dis;

      if (ID2 == ID1) {

        new_label.emplace_back(ID1, 0, 0);
        continue;
      }

      if (nodeID_label_dis[ID2] != d1) {
        continue;
      }

      int d2 = INF;
      // auto &left = label[ID1], &right = label[ID2];
      auto &right = label[ID2];

      vector<int> commonHub;
      for (int j = 0; j < right.size(); ++j) {
        int hub2 = right[j].hub;
        if (nodeID_label_dis[hub2] != INF) {
          int tmp = nodeID_label_dis[hub2] + right[j].dis;
          if (tmp < d2) {
            d2 = tmp;
            commonHub.clear();
            commonHub.push_back(hub2);
          } else if (tmp == d2) {
            commonHub.push_back(hub2);
          }
        }
      }

      if (d1 != d2) {
        continue;
      } else {
        if (commonHub.size() == 1 && commonHub[0] == ID2) {

          new_label.emplace_back(ID2, d2, label[ID1][j].step);
#pragma omp critical
          {
            int tmp_step = label[ID1][j].step;
            maxStep = max(maxStep, tmp_step);
          }
        }
      }
    }

    for (int i = 0; i < left.size(); ++i) {
      nodeID_label_dis[left[i].hub] = INF;
    }
  }
  swap(label, new_label_vec);
}

void Build::removeRedundantLabel(int topK) {

  vector<vector<Label>> new_label_vec(nodenum);

  record_nodeID_label_dis.clear();
  record_nodeID_label_dis.resize(threadnum, vector<int>(nodenum, INF));
// #pragma omp parallel for num_threads(threadnum)
#pragma omp parallel for num_threads(threadnum) schedule(dynamic)
  // for (int ID1 = 0; ID1 < nodenum; ID1++) {
  for (int idx = nodenum - 1; idx >= 0; --idx) {
    int ID1 = vNodeOrder[idx];
    int thread_id = omp_get_thread_num();
    auto &new_label = new_label_vec[ID1];

    if (is_topK[ID1] == 1) {
      new_label = label[ID1];
      continue;
    }

    auto &nodeID_label_dis = record_nodeID_label_dis[thread_id];
    auto &left = label[ID1];
    for (int i = 0; i < left.size(); ++i) {
      if (left[i].dis < nodeID_label_dis[left[i].hub]) {
        nodeID_label_dis[left[i].hub] = left[i].dis;
      }
      // nodeID_label_dis[left[i].hub] = left[i].dis;
    }

    for (int j = 0; j < label[ID1].size(); j++) {

      int ID2 = label[ID1][j].hub, d1 = label[ID1][j].dis;

      if (ID2 == ID1) {

        new_label.emplace_back(ID1, 0, 0);
        continue;
      }

      if (is_topK[ID2] == 1) {

        new_label.emplace_back(ID2, d1, label[ID1][j].step);
        continue;
      }

      if (nodeID_label_dis[ID2] != d1) {
        continue;
      }

      int d2 = INF;
      // auto &left = label[ID1], &right = label[ID2];
      auto &right = label[ID2];

      vector<int> commonHub;
      for (int j = 0; j < right.size(); ++j) {
        int hub2 = right[j].hub;
        if (nodeID_label_dis[hub2] != INF) {
          int tmp = nodeID_label_dis[hub2] + right[j].dis;
          if (tmp < d2) {
            d2 = tmp;
            commonHub.clear();
            commonHub.push_back(hub2);
          } else if (tmp == d2) {
            commonHub.push_back(hub2);
          }
        }
      }

      if (d1 != d2) {
        continue;
      } else {
        if (commonHub.size() == 1 && commonHub[0] == ID2) {

          new_label.emplace_back(ID2, d2, label[ID1][j].step);
        }
      }
    }

    for (int i = 0; i < left.size(); ++i) {
      nodeID_label_dis[left[i].hub] = INF;
    }
  }
  swap(label, new_label_vec);
}
