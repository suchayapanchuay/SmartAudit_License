#
# Copyright (c) 2020 WALLIX, SAS. All rights reserved.
# Licensed computer software. Property of WALLIX.
# Author(s) : Meng Tan <mtan@wallix.com>
# URL: $URL$
# Module description:
# Compile with:
#

from time import time as get_time
from enum import Enum

from typing import Optional, Dict


DEBUG = False


class Steps(Enum):
    PRIMARY_CONN = "primary_connection"
    PRIMARY_AUTH = "primary_authentication"
    FETCH_RIGHTS = "fetch_rights"
    CHECKOUT_TARGET = "checkout_target"
    TARGET_CONN = "target_connection"


if DEBUG:
    def print_debug(msg):
        print(msg)
else:
    def print_debug(msg):
        pass  # disable debug


class Logtime:
    saved_times: Dict[Steps, float]
    current_step: Optional[Steps]
    last_time: float
    step_time: float
    paused: bool

    def __init__(self):
        self.saved_times = {}
        self._reset()

    def _reset(self):
        self.current_step = None
        self.last_time = 0
        self.step_time = 0
        self.paused = False

    def _add_time(self, current_time: float) -> None:
        diff_time = (current_time - self.last_time)
        print_debug(f">>>>> Time ADDED {diff_time} for state {self.current_step}")
        self.step_time += diff_time
        self.last_time = current_time

    def _save_time(self) -> None:
        print_debug(f">>>>> Time SAVED {self.step_time} for state {self.current_step}")
        self.saved_times[self.current_step] = round(self.step_time, 3)
        self._reset()

    def start(self, step: Steps, current_time: float = 0) -> None:
        current_time = current_time or get_time()
        print_debug(f">>>>> START {step}")
        if self.current_step == step:
            # step already started
            # resume it if it was paused
            self.resume(step, current_time)
            return
        if self.current_step is not None:
            # stop current step
            self.stop(step=self.current_step, current_time=current_time)
        self.current_step = step
        self.last_time = current_time

    def stop(self, step: Optional[Steps] = None, current_time: float = 0) -> None:
        print_debug(f">>>>> STOP {step or self.current_step}")
        if step is not None and step != self.current_step:
            return
        if self.current_step is not None:
            if not self.paused:
                current_time = current_time or get_time()
                self._add_time(current_time)
            self._save_time()

    def pause(self, step: Optional[Steps] = None, current_time: float = 0) -> None:
        print_debug(f">>>>> PAUSE {step or self.current_step}")
        if step is not None and step != self.current_step:
            # ignore this call as the step is not concerned
            return
        if self.current_step and not self.paused:
            current_time = current_time or get_time()
            self._add_time(current_time)
            self.paused = True
        print_debug(self.saved_times)

    def resume(self, step: Optional[Steps] = None, current_time: float = 0) -> None:
        print_debug(f">>>>> RESUME {step or self.current_step}")
        if step is not None and step != self.current_step:
            # ignore this call as the step is not concerned
            return
        if self.current_step and self.paused:
            current_time = current_time or get_time()
            self.last_time = current_time
            self.paused = False

    def is_paused(self) -> bool:
        return self.paused

    def is_started(self, step: Optional[Steps] = None) -> bool:
        if step is None:
            return self.current_step is not None
        return self.current_step == step

    def total_metrics(self) -> float:
        total = sum(self.saved_times.values())
        return round(total, 3)

    def report_metrics(self) -> Dict[str, float]:
        metrics = {
            step.value: value
            for step, value in self.saved_times.items()
        }
        metrics['total'] = self.total_metrics()
        return metrics

    def add_step_time(self, step: Steps, step_time: float) -> None:
        self.saved_times[step] = round(step_time, 3)


logtimer = Logtime()


def logtime_prim_auth(func):
    def cb_auth_wrapper(*args, **kwargs):
        logtimer.resume(Steps.PRIMARY_AUTH)
        res = func(*args, **kwargs)
        logtimer.pause(Steps.PRIMARY_AUTH)
        return res
    return cb_auth_wrapper


def logtime_function_resume(func):
    def wrapper(*args, **kwargs):
        init_paused = logtimer.is_paused()
        init_started = logtimer.is_started()
        if init_paused:
            logtimer.resume()
        res = func(*args, **kwargs)
        just_started = not init_started and logtimer.is_started()
        if init_paused or just_started:
            logtimer.pause()
        return res
    return wrapper


def logtime_function_pause(func):
    def wrapper(*args, **kwargs):
        logtimer.pause()
        res = func(*args, **kwargs)
        logtimer.resume()
        return res
    return wrapper


def logtime_function_step_resume(step):
    def real_decorator(func):
        def wrapper(*args, **kwargs):
            init_paused = logtimer.is_paused()
            init_started = logtimer.is_started(step)
            if init_paused:
                logtimer.resume(step)
            res = func(*args, **kwargs)
            just_started = not init_started and logtimer.is_started(step)
            if init_paused or just_started:
                logtimer.pause(step)
            return res
        return wrapper
    return real_decorator


def logtime_function_step_pause(step):
    def real_decorator(func):
        def wrapper(*args, **kwargs):
            logtimer.pause(step)
            res = func(*args, **kwargs)
            logtimer.resume(step)
            return res
        return wrapper
    return real_decorator
