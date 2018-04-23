#ifndef _TIMER_H_
#define _TIMER_H_

#include <chrono>

class timer {
private:
	std::chrono::high_resolution_clock::time_point tpstart;
public:
	void start();
	double peek();
	double end();
};

#endif
