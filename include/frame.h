#ifndef FRAME_H
#define FRAME_H

#include <memory>

#include "scenegraph.h"

using namespace std;

class Frame {
  public:
      void capture(shared_ptr<SgNode> root);
      void restore();

      string serialize();
      void deserialize(shared_ptr<SgNode> root, const string &serialized);

      friend Frame interpolate(const Frame &first, const Frame &second, double alpha);
      friend Frame catmullRomInterpolate(const Frame &f0, const Frame &f1, const Frame &f2, const Frame &f3, double alpha);

  private:
      vector<RigTForm> frameRbts_;
      vector<shared_ptr<SgRbtNode> > nodePtrs_;

      static vector<RigTForm> parse(const string &serialized);
};

Frame interpolate(const Frame &first, const Frame &second, double alpha);
Frame catmullRomInterpolate(const Frame &f0, const Frame &f1, const Frame &f2, const Frame &f3);

#endif//FRAME_H
