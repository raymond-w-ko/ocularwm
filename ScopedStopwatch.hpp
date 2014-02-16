#pragma once

class ScopedStopwatch {
 public:
  typedef boost::chrono::high_resolution_clock clock;
  ScopedStopwatch(std::string name) : mName(name) {
    mBeginTime = clock::now();
  }
  ~ScopedStopwatch() {
    boost::chrono::nanoseconds ns = clock::now() - mBeginTime;
    double secs = static_cast<double>(ns.count()) / 1e9;
    char buffer[256];
    sprintf(buffer, "%s: %f\n", mName.c_str(), secs);
    Trace(buffer);
  }

 private:
  std::string mName;
  clock::time_point mBeginTime;
};
