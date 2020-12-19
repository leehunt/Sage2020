#pragma once
#include <cassert>

// A lightweight mix-in listener interface.
class Sage2020ViewDocListener {
 public:
  Sage2020ViewDocListener()
      : next_listener_(nullptr), previous_listener_(nullptr) {}
  ~Sage2020ViewDocListener() {
    assert(next_listener_ == nullptr);
    assert(previous_listener_ == nullptr);
  }

  virtual void DocEditNotification(int iLine, int cLine) = 0;
  virtual void DocVersionChangedNotification(int nVer) = 0;

 protected:
  Sage2020ViewDocListener* next_listener_;
  Sage2020ViewDocListener* previous_listener_;
};
