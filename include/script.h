#ifndef SCRIPT_H
#define SCRIPT_H


#include <list>

#include "frame.h"

class Script {
  public:
      void initialize(shared_ptr<SgNode> sceneGraphRoot);

      void restoreCurFrame();

      void overwriteCurFrame();

      void advanceCurFrame();
      void retreatCurFrame();

      void deleteCurFrame();

      void createNewFrame();

      void save(const string &path);
      void load(const string &path);

      size_t getFrameCount() { return frames_.size(); }

      void reset(size_t index = 0);
      void restoreInterpolatedFrame(double alpha);

  public:
      friend ostream &operator<<(ostream &out, const Script &script);

  private:
      list<Frame> frames_;
      list<Frame>::iterator iter_;

      shared_ptr<SgNode> sgRoot_;

};


#endif//SCRIPT_H
