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

Frame interpolate(const Frame &first, const Frame &second, double alpha) {
  assert(first.frameRbts_.size() == second.frameRbts_.size());
  assert(first.nodePtrs_.size() == second.nodePtrs_.size());

  Frame ret;

  for (int i = 0; i < first.frameRbts_.size(); i++) {
    const RigTForm &firstRbt = first.frameRbts_[i];
    const RigTForm &secondRbt = second.frameRbts_[i];

    ret.frameRbts_.push_back(
            RigTForm(
            lerp(firstRbt.getTranslation(), secondRbt.getTranslation(), alpha),
            slerp(firstRbt.getRotation(), secondRbt.getRotation(), alpha))
            );

    assert(*first.nodePtrs_[i] == *second.nodePtrs_[i]);
    ret.nodePtrs_.push_back(first.nodePtrs_[i]);
  }

  return ret;
}

Frame catmullRomInterpolate(const Frame &f0, const Frame &f1, const Frame &f2, const Frame &f3, double alpha) {
  Frame ret;
  for (int i = 0; i < f0.frameRbts_.size(); i++) {
    const RigTForm &rbt0 = f0.frameRbts_[i];
    const RigTForm &rbt1 = f1.frameRbts_[i];
    const RigTForm &rbt2 = f2.frameRbts_[i];
    const RigTForm &rbt3 = f3.frameRbts_[i];

    ret.frameRbts_.push_back(catmullRomInterpolate(rbt0, rbt1, rbt2, rbt3, alpha));
    ret.nodePtrs_.push_back(f0.nodePtrs_[i]);
  }

  return ret;
}
