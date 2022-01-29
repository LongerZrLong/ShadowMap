#include "script.h"

#include <fstream>


void Script::initialize(shared_ptr<SgNode> sceneGraphRoot) {
  frames_.clear();
  iter_ = frames_.begin();

  sgRoot_ = sceneGraphRoot;
}

void Script::restoreCurFrame() {
  if (iter_ != frames_.end()) {
    cout << "Script::restoreCurFrame" << endl;
    iter_->restore();
  }
}

void Script::overwriteCurFrame() {
  if (iter_ != frames_.end()) {
    cout << "Script::overwriteCurFrame" << endl;
    // create a new frame and capture the scene
    Frame frame = Frame();
    frame.capture(sgRoot_);

    // overwrite the current and
    *iter_ = frame;
  } else {
    createNewFrame();
  }
}

void Script::advanceCurFrame() {
  if (iter_ != frames_.end()) {
    cout << "Script::advanceCurFrame" << endl;
    iter_++;
    if (iter_ != frames_.end())
      iter_->restore();
  }
}

void Script::retreatCurFrame() {
  if (iter_ != frames_.begin()) {
    cout << "Script::retreatCurFrame" << endl;
    iter_--;
    iter_->restore();
  }
}

void Script::deleteCurFrame() {
  if (iter_ == frames_.end())
    return;

  cout << "Script::deleteCurFrame" << endl;
  auto tmp = iter_;
  if (tmp == frames_.begin()) {
    // delete the first frame, set current frame to the next frame
    tmp++;
  } else {
    // delete the frame in the middle, set current frame to the previous frame
    tmp--;
  }

  frames_.erase(iter_);
  iter_ = tmp;

  if (iter_ != frames_.end())
    iter_->restore();
}

void Script::createNewFrame() {
  cout << "Script::createNewFrame" << endl;

  Frame frame = Frame();
  frame.capture(sgRoot_);

  if (iter_ != frames_.end()) {
    auto tmp = iter_;
    tmp++;
    frames_.insert(tmp, frame);

    iter_++;
  } else {
    // iter points to frames_.end()
    frames_.insert(iter_, frame);
    iter_--;
    iter_->restore();
  }
}

void Script::save(const string &path) {
  cout << "Script::save" << endl;
  ofstream fs(path);

  if (!fs.is_open()) {
    cerr << "Unable to open file: " << path << endl;
    return;
  }

  for (auto &frame : frames_) {
    fs << frame.serialize() << "\n";
  }
  fs.close();
}

void Script::load(const string &path) {
  cout << "Script::load" << endl;

  ifstream fs(path);

  if (!fs.is_open()) {
    cerr << "Unable to open file: " << path << endl;
    return;
  }

  frames_.clear();
  iter_ = frames_.begin();

  string line;
  while (getline(fs, line, '\n')) {
    Frame frame;
    frame.deserialize(sgRoot_, line);

    frames_.push_back(frame);
  }
}

void Script::reset(size_t index) {
  if (index > frames_.size() - 1) return;

  iter_ = frames_.begin();
  for (size_t i = 0; i < index; i++) {
    iter_++;
  }

  iter_->restore();
}

void Script::restoreInterpolatedFrame(double alpha) {
  auto tmp = iter_;
  tmp--;
  Frame f0 = *tmp;

  tmp++;
  Frame f1 = *tmp;

  tmp++;
  Frame f2 = *tmp;

  tmp++;
  Frame f3 = *tmp;

  catmullRomInterpolate(f0, f1, f2, f3, alpha).restore();
}

ostream &operator<<(ostream &out, const Script &script) {
  out << "Number of Frame: " <<script.frames_.size();
  return out;
}
