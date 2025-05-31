#include "utils/timer.h"

#include "base/threading/sequenced_task_runner_handle.h"
#include "base/time/time_ticks.h"

namespace util {

Timer::Timer() : weak_factory_(this) {}

Timer::~Timer() = default;

void Timer::Start(base::RepeatingClosure callback, base::TimeDelta interval) {
  weak_factory_.InvalidateWeakPtrs();
  weak_this_ = weak_factory_.GetWeakPtr();

  callback_ = base::BindRepeating(&Timer::RunAndScheduleNext, weak_this_,
                                  std::move(callback), interval);
  callback_.Run();
}

void Timer::Stop() {
  callback_ = base::RepeatingClosure();
  weak_factory_.InvalidateWeakPtrs();
}

bool Timer::IsRunning() const {
  return !!callback_;
}

void Timer::RunAndScheduleNext(base::RepeatingClosure callback,
                               base::TimeDelta interval) {
  const auto start = base::TimeTicks::Now();
  callback.Run();

  const auto duration = base::TimeTicks::Now() - start;
  auto next = interval - duration;
  if (next < base::Seconds(0)) {
    next = base::Seconds(0);
  }

  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(FROM_HERE, callback_,
                                                          next);
}

}  // namespace util
