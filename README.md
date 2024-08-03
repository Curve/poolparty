<p align="center">
  <img src="assets/logo.svg" width="600">
</p>

## 📃 Description

_poolparty_ is a simple and versatile C++20 thread-pool library.

## 📦 Installation

* Using [CPM](https://github.com/cpm-cmake/CPM.cmake)
  ```cmake
  CPMFindPackage(
    NAME           poolparty
    VERSION        3.0.0
    GIT_REPOSITORY "https://github.com/Curve/poolparty"
  )
  ```

* Using FetchContent
  ```cmake
  include(FetchContent)

  FetchContent_Declare(poolparty GIT_REPOSITORY "https://github.com/Curve/poolparty" GIT_TAG v3.0.0)
  FetchContent_MakeAvailable(poolparty)

  target_link_libraries(<target> cr::poolparty)
  ```

## 📖 Examples

```cpp
#include <iostream>
#include <poolparty/pool.hpp>

int expensive_calculation(int x)
{
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return x + 1;
}

int main()
{
    poolparty::pool pool{2};

    // Pause execution
    pool.pause();

    auto fut1 = pool.submit(expensive_calculation, 1);
    auto fut2 = pool.submit(expensive_calculation, 2);

    // Let's add an additional thread
    auto source = pool.add_thread();
    auto fut3   = pool.submit(expensive_calculation, 3);

    // Fire and forget
    pool.forget([]() { expensive_calculation(100); });

    // Resume execution
    pool.resume();

    std::cout << "Result: " << fut1.get() << ", " << fut2.get() << " and " << fut3.get() << std::endl;

    // ...and remove the thread we've added
    source.request_stop();
    pool.cleanup();

    return 0;
}
```

It's also possible to use a priority queue for more fine grained execution, see [tests](tests/priority.test.cpp):

https://github.com/Curve/poolparty/blob/4f58b719c99238985c1e99be6b07f893cfbd9c50/tests/priority.test.cpp#L48-L62
