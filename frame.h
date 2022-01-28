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

  private:
      vector<RigTForm> frameRbts_;
      vector<shared_ptr<SgRbtNode> > nodePtrs_;

      static vector<RigTForm> parse(const string &serialized);
};


#endif//FRAME_H
