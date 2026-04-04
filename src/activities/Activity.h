#pragma once

#include <Logging.h>

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "ActivityManager.h"
#include "GfxRenderer.h"
#include "MappedInputManager.h"
#include "RenderLock.h"

class Activity {
  friend class ActivityManager;

 protected:
  std::string name;
  GfxRenderer& renderer;
  MappedInputManager& mappedInput;

  std::function<void(bool cancelled)> resultHandler;
  bool resultCancelled = false;

 public:
  explicit Activity(std::string name, GfxRenderer& renderer, MappedInputManager& mappedInput)
      : name(std::move(name)), renderer(renderer), mappedInput(mappedInput) {}
  virtual ~Activity() = default;
  virtual void onEnter();
  virtual void onExit();
  virtual void loop() {}
  virtual void render(RenderLock&&) {}
  virtual void requestUpdate(bool immediate = false);
  virtual void requestUpdateAndWait();
  virtual bool skipLoopDelay() { return false; }
  virtual bool preventAutoSleep() { return false; }

  void startActivityForResult(std::unique_ptr<Activity>&& activity,
                              std::function<void(bool cancelled)> handler);
  void setResultCancelled(bool cancelled);
  void finish();
};
