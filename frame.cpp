#include "frame.h"

#include <sstream>

#include "sgutils.h"


void Frame::capture(shared_ptr<SgNode> root) {
  frameRbts_.clear();
  nodePtrs_.clear();

  dumpSgRbtNodes(root, nodePtrs_);
  for (auto ptr : nodePtrs_) {
    frameRbts_.push_back(ptr->getRbt());
  }
}

void Frame::restore() {
  for (int i = 0; i < nodePtrs_.size(); i++) {
    nodePtrs_[i]->setRbt(frameRbts_[i]);
  }
}

string Frame::serialize() {
  stringstream ss;
  for (const auto &rbt : frameRbts_) {
    ss << rbt;
  }
  return ss.str();
}

void Frame::deserialize(shared_ptr<SgNode> root, const string &serialized) {
  frameRbts_.clear();
  nodePtrs_.clear();

  dumpSgRbtNodes(root, nodePtrs_);
  frameRbts_ = parse(serialized);
}

vector<RigTForm> Frame::parse(const string &serialized) {
  vector<RigTForm> ret;

  string rbt, ele;
  double buf[7];
  stringstream ss(serialized);

  while (getline(ss, rbt, ';')) {
    stringstream rbtss(rbt);

    int count = 0;
    while (getline(rbtss, ele, ',')) {
      buf[count] = stod(ele);
      count++;
    }

    ret.push_back(RigTForm(
                    Cvec3(buf[0], buf[1], buf[2]),
                    Quat(buf[3], buf[4], buf[5], buf[6])));
  }

  return ret;
}