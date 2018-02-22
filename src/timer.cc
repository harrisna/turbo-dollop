#include <chrono>

#include "timer.h"

void timer::start() {
	tpstart = std::chrono::high_resolution_clock::now();
}

double timer::end() {
	std::chrono::high_resolution_clock::time_point tpend = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> time = (tpend - tpstart);
	return time.count();
}
