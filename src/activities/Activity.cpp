#include "Activity.h"

#include "ActivityManager.h"

void Activity::onEnter() { LOG_DBG("ACT", "Entering activity: %s", name.c_str()); }

void Activity::onExit() { LOG_DBG("ACT", "Exiting activity: %s", name.c_str()); }

void Activity::requestUpdate(bool immediate) { activityManager.requestUpdate(immediate); }

void Activity::requestUpdateAndWait() { activityManager.requestUpdateAndWait(); }

void Activity::startActivityForResult(std::unique_ptr<Activity>&& activity,
                                      std::function<void(bool cancelled)> handler) {
  this->resultHandler = std::move(handler);
  activityManager.pushActivity(std::move(activity));
}

void Activity::setResultCancelled(bool cancelled) { this->resultCancelled = cancelled; }

void Activity::finish() { activityManager.popActivity(); }
