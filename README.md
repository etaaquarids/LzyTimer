# LzyTimer
### Get Start
use case as seperater timer
``` C++
auto timer = Lzy::Timer::Timer(8ms);
timer.start();
timer.setTimeout([](){}, 500ms);
```
use case in game loop
``` C++
auto timer = Lzy::Timer::Timer(10ms);
timer.setTimeout([](){}, 500ms);
while(true){
    auto time_to_awake = timer.tick_once();
    // other ticks
    ...
    sleep_until(time_to_awake);
}
```
time wheel size can be assigned like
``` C++
auto timer = Lzy::Timer::Timer(10ms, 128, 128, 128, ...);
```
start and tick_once can receive an executor function to do the functions.
``` C++
timer.start([](auto&& fn){ Executor::execute(fn); });
```
or
``` C++
timer.tick_once(Executor::execute<FunctionType>);
```
### benchmark
