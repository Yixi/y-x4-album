#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <cassert>
#include <memory>
#include <string>
#include <vector>

#include "GfxRenderer.h"
#include "MappedInputManager.h"

class Activity;
class RenderLock;

class ActivityManager {
  friend class RenderLock;

 protected:
  GfxRenderer& renderer;
  MappedInputManager& mappedInput;
  std::vector<std::unique_ptr<Activity>> stackActivities;
  std::unique_ptr<Activity> currentActivity;

  void exitActivity(const RenderLock& lock);

  std::unique_ptr<Activity> pendingActivity;
  enum class PendingAction { None, Push, Pop, Replace };
  PendingAction pendingAction = PendingAction::None;

  TaskHandle_t renderTaskHandle = nullptr;
  static void renderTaskTrampoline(void* param);
  [[noreturn]] virtual void renderTaskLoop();

  TaskHandle_t waitingTaskHandle = nullptr;

  SemaphoreHandle_t renderingMutex = nullptr;

  bool requestedUpdate = false;

 public:
  explicit ActivityManager(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : renderer(renderer), mappedInput(mappedInput), renderingMutex(xSemaphoreCreateMutex()) {
    assert(renderingMutex != nullptr && "Failed to create rendering mutex");
    stackActivities.reserve(5);
  }
  ~ActivityManager() { assert(false); }

  void begin();
  void loop();

  void replaceActivity(std::unique_ptr<Activity>&& newActivity);

  // Album-specific navigation
  void goToBoot();
  void goToGallery();
  void goToGallery(const char* dirPath);
  void goToSleep();

  void pushActivity(std::unique_ptr<Activity>&& activity);
  void popActivity();

  bool preventAutoSleep() const;
  bool skipLoopDelay() const;

  void requestUpdate(bool immediate = false);
  void requestUpdateAndWait();
};

extern ActivityManager activityManager;
