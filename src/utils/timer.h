#pragma once

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time_delta.h"

namespace util {

class Timer {
 public:
  Timer();
  ~Timer();

  void Start(base::RepeatingClosure callback, base::TimeDelta interval);
  void Stop();

  bool IsRunning() const;

 private:
  void RunAndScheduleNext(base::RepeatingClosure callback,
                          base::TimeDelta interval);

  base::RepeatingClosure callback_;

  base::WeakPtr<Timer> weak_this_;
  base::WeakPtrFactory<Timer> weak_factory_;
};

}  // namespace util
