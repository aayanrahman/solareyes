CXX ?= g++
CXXFLAGS ?= -std=c++11 -Wall -Wextra -Werror -I../firmware/SolarEyes

.PHONY: test clean

test: test_eyes
	./test_eyes

test_eyes: test_eyes.cpp ../firmware/SolarEyes/eyes.cpp ../firmware/SolarEyes/eyes.h ../firmware/SolarEyes/config.h
	$(CXX) $(CXXFLAGS) -o test_eyes test_eyes.cpp ../firmware/SolarEyes/eyes.cpp

clean:
	rm -f test_eyes
