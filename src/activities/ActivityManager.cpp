#include "ActivityManager.h"

#include <HalPowerManager.h>

#include "Activity.h"
#include "BootActivity.h"
#include "GalleryActivity.h"
#include "RenderLock.h"
#include "components/AlbumTheme.h"

void ActivityManager::begin() {
  xTaskCreate(&renderTaskTrampoline, "AlbumRender",
              8192,
              this,
              1,
              &renderTaskHandle);
  assert(renderTaskHandle != nullptr && "Failed to create render task");
}

void ActivityManager::renderTaskTrampoline(void* param) {
  auto* self = static_cast<ActivityManager*>(param);
  self->renderTaskLoop();
}

void ActivityManager::renderTaskLoop() {
  while (true) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    RenderLock lock;
    if (currentActivity) {
      HalPowerManager::Lock powerLock;
      currentActivity->render(std::move(lock));
    }
    TaskHandle_t waiter = nullptr;
    taskENTER_CRITICAL(nullptr);
    waiter = waitingTaskHandle;
    waitingTaskHandle = nullptr;
    taskEXIT_CRITICAL(nullptr);
    if (waiter) {
      xTaskNotify(waiter, 1, eIncrement);
    }
  }
}

void ActivityManager::loop() {
  if (currentActivity) {
    currentActivity->loop();
  }

  while (pendingAction != PendingAction::None) {
    if (pendingAction == PendingAction::Pop) {
      RenderLock lock;

      if (!currentActivity) {
        LOG_ERR("ACT", "Pop set but currentActivity is null");
        pendingAction = PendingAction::None;
        continue;
      }

      bool pendingCancelled = currentActivity->resultCancelled;

      exitActivity(lock);
      pendingAction = PendingAction::None;

      if (stackActivities.empty()) {
        LOG_DBG("ACT", "No more activities on stack, going to gallery");
        lock.unlock();
        goToGallery();
        continue;
      } else {
        currentActivity = std::move(stackActivities.back());
        stackActivities.pop_back();
        LOG_DBG("ACT", "Popped from activity stack, new size = %zu", stackActivities.size());
        if (currentActivity->resultHandler) {
          auto handler = std::move(currentActivity->resultHandler);
          currentActivity->resultHandler = nullptr;
          lock.unlock();
          handler(pendingCancelled);
        }
        if (pendingAction == PendingAction::None) {
          requestUpdate();
        }
        continue;
      }

    } else if (pendingActivity) {
      RenderLock lock;

      if (pendingAction == PendingAction::Replace) {
        exitActivity(lock);
        while (!stackActivities.empty()) {
          stackActivities.back()->onExit();
          stackActivities.pop_back();
        }
      } else if (pendingAction == PendingAction::Push) {
        stackActivities.push_back(std::move(currentActivity));
        LOG_DBG("ACT", "Pushed to activity stack, new size = %zu", stackActivities.size());
      }
      pendingAction = PendingAction::None;
      currentActivity = std::move(pendingActivity);

      lock.unlock();
      currentActivity->onEnter();
      continue;
    }
  }

  if (requestedUpdate) {
    requestedUpdate = false;
    if (renderTaskHandle) {
      xTaskNotify(renderTaskHandle, 1, eIncrement);
    }
  }
}

void ActivityManager::exitActivity(const RenderLock&) {
  if (currentActivity) {
    currentActivity->onExit();
    currentActivity.reset();
  }
}

void ActivityManager::replaceActivity(std::unique_ptr<Activity>&& newActivity) {
  if (currentActivity) {
    pendingActivity = std::move(newActivity);
    pendingAction = PendingAction::Replace;
  } else {
    currentActivity = std::move(newActivity);
    currentActivity->onEnter();
  }
}

void ActivityManager::goToBoot() {
  replaceActivity(std::make_unique<BootActivity>(renderer, mappedInput));
}

void ActivityManager::goToGallery() {
  replaceActivity(std::make_unique<GalleryActivity>(renderer, mappedInput));
}

void ActivityManager::goToGallery(const char* dirPath) {
  replaceActivity(std::make_unique<GalleryActivity>(renderer, mappedInput, dirPath));
}

void ActivityManager::goToSleep() {
  LOG_INF("ACT", "Sleep requested — showing shutdown screen");
  RenderLock lock;
  THEME.drawShutdownScreen(renderer);
  renderer.displayBuffer(HalDisplay::FULL_REFRESH);
}

void ActivityManager::pushActivity(std::unique_ptr<Activity>&& activity) {
  if (pendingActivity) {
    LOG_ERR("ACT", "pendingActivity while pushActivity is not expected");
    pendingActivity.reset();
  }
  pendingActivity = std::move(activity);
  pendingAction = PendingAction::Push;
}

void ActivityManager::popActivity() {
  if (pendingActivity) {
    LOG_ERR("ACT", "pendingActivity while popActivity is not expected");
    pendingActivity.reset();
  }
  pendingAction = PendingAction::Pop;
}

bool ActivityManager::preventAutoSleep() const { return currentActivity && currentActivity->preventAutoSleep(); }

bool ActivityManager::skipLoopDelay() const { return currentActivity && currentActivity->skipLoopDelay(); }

void ActivityManager::requestUpdate(bool immediate) {
  if (immediate) {
    if (renderTaskHandle) {
      xTaskNotify(renderTaskHandle, 1, eIncrement);
    }
  } else {
    requestedUpdate = true;
  }
}

void ActivityManager::requestUpdateAndWait() {
  if (!renderTaskHandle) {
    return;
  }

  taskENTER_CRITICAL(nullptr);
  auto currTaskHandler = xTaskGetCurrentTaskHandle();
  auto mutexHolder = xSemaphoreGetMutexHolder(renderingMutex);
  bool isRenderTask = (currTaskHandler == renderTaskHandle);
  bool alreadyWaiting = (waitingTaskHandle != nullptr);
  bool holdingRenderLock = (mutexHolder == currTaskHandler);
  if (!alreadyWaiting && !isRenderTask && !holdingRenderLock) {
    waitingTaskHandle = currTaskHandler;
  }
  taskEXIT_CRITICAL(nullptr);

  assert(!isRenderTask && "Render task cannot call requestUpdateAndWait()");
  assert(!alreadyWaiting && "Already waiting for a render to complete");
  assert(!holdingRenderLock && "Cannot call requestUpdateAndWait() while holding RenderLock");

  xTaskNotify(renderTaskHandle, 1, eIncrement);
  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
}

// RenderLock implementation

RenderLock::RenderLock() {
  xSemaphoreTake(activityManager.renderingMutex, portMAX_DELAY);
  isLocked = true;
}

RenderLock::RenderLock([[maybe_unused]] Activity&) {
  xSemaphoreTake(activityManager.renderingMutex, portMAX_DELAY);
  isLocked = true;
}

RenderLock::~RenderLock() {
  if (isLocked) {
    xSemaphoreGive(activityManager.renderingMutex);
    isLocked = false;
  }
}

void RenderLock::unlock() {
  if (isLocked) {
    xSemaphoreGive(activityManager.renderingMutex);
    isLocked = false;
  }
}

bool RenderLock::peek() { return xSemaphoreGetMutexHolder(activityManager.renderingMutex) != nullptr; }
