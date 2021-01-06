#include "pch.h"

#include "Sage2020ViewDocListener.h"

Sage2020ViewDocListener* Sage2020ViewDocListener::LinkListener(
    Sage2020ViewDocListener* listener_head) {
  assert(next_listener_ == nullptr);
  assert(previous_listener_ == nullptr);
  if (listener_head != nullptr) {
    assert(listener_head->previous_listener_ == nullptr);
    listener_head->previous_listener_ = this;
  }

  next_listener_ = listener_head;

  return this;
}

Sage2020ViewDocListener* Sage2020ViewDocListener::UnlinkListener(
    Sage2020ViewDocListener* listener_head) {

  assert(listener_head != nullptr);
  assert(listener_head != next_listener_);

  if (next_listener_ != nullptr) {
    assert(next_listener_->previous_listener_ == this);
    next_listener_->previous_listener_ = previous_listener_;
  }

  Sage2020ViewDocListener* new_head;
  if (previous_listener_ != nullptr) {
    assert(previous_listener_->next_listener_ == this);
    previous_listener_->next_listener_ = next_listener_;

    new_head = listener_head;
  } else {
    assert(listener_head == this);

    new_head = next_listener_;
  }

  next_listener_ = nullptr;
  previous_listener_ = nullptr;

  return new_head;
}


void Sage2020ViewDocListener::NotifyAllListenersOfEdit(int iLine, int cLine) {
  assert(this->previous_listener_ == nullptr);

  Sage2020ViewDocListener* listener = this;
  while (listener != NULL) {
    listener->DocEditNotification(iLine, cLine);
    listener = listener->next_listener_;
  }
}

void Sage2020ViewDocListener::NotifyAllListenersOfVersionChange(size_t nVer) {
  assert(this->previous_listener_ == nullptr);

  Sage2020ViewDocListener* listener = this;
  while (listener != NULL) {
    listener->DocVersionChangedNotification(nVer);
    listener = listener->next_listener_;
  }
}
