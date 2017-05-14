# Theater Simulation

## Summary
  * Modelled customers' and employees' behavior of a theater by using threads - one thread for each person
  * Created, terminated threads and fetched results returned by threads by using facilities in [C++11][cpp11] [Thread Support Library][tsl] (i.e. [`std::thread`][std_thread], [`std::promise`][std_promise], [`std::future`][std_future], etc)
  * Coordinated each person's activity (aka each thread) by using semaphores (A semaphore is implemented by a [`std::condition_variable`][std_cv] and a [`std::mutex`][std_mutex])

## Project Information
  * Course: [Operating Systems Concepts (CS 5348)][os]
  * Professor: [Greg Ozbirn][ozbirn]
  * Semester: Spring 2016
  * Programming Language: C++
  * Build Tool: [CMake][cmake]

[cpp11]: https://en.wikipedia.org/wiki/C%2B%2B11
[std_thread]: http://en.cppreference.com/w/cpp/thread/thread
[std_promise]: http://en.cppreference.com/w/cpp/thread/promise
[std_future]: http://en.cppreference.com/w/cpp/thread/future
[std_cv]: http://en.cppreference.com/w/cpp/thread/condition_variable
[std_mutex]: http://en.cppreference.com/w/cpp/thread/mutex
[tsl]: http://en.cppreference.com/w/cpp/thread
[os]: https://catalog.utdallas.edu/2016/graduate/courses/cs5348
[cmake]: https://cmake.org/
[ozbirn]: http://cs.utdallas.edu/people/faculty/ozbirn-greg/
