#pragma once
#include <cassert>

// A lightweight mix-in listener interface.
class DiffTreePath;
class Sage2020ViewDocListener {
 public:
  Sage2020ViewDocListener()
      : next_listener_(nullptr), previous_listener_(nullptr) {}
  ~Sage2020ViewDocListener() {
    assert(next_listener_ == nullptr);
    assert(previous_listener_ == nullptr);
  }

  virtual void DocEditNotification(int iLine, int cLine) = 0;
  virtual void DocVersionChangedNotification(const DiffTreePath& tree_path) = 0;

  Sage2020ViewDocListener* LinkListener(Sage2020ViewDocListener* listener_head);
  Sage2020ViewDocListener* UnlinkListener(
      Sage2020ViewDocListener* listener_head);

  void NotifyAllListenersOfEdit(int iLine, int cLine);
  void NotifyAllListenersOfVersionChange(const DiffTreePath& tree_path);

 protected:
  Sage2020ViewDocListener* next_listener_;
  Sage2020ViewDocListener* previous_listener_;
};
