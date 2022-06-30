#!/bin/bash

g++ --version

mkdir -p Release
g++ -Wall -std=c++14 -D_NDEBUG -O3 -lpthread -g bigsort.cpp -o Release/bigsort
g++ -Wall -std=c++14 -D_NDEBUG -O3 -lpthread -g bigsort_test.cpp -o Release/bigsort_test
g++ -Wall -std=c++14 -D_NDEBUG -O3 -lpthread -g makebad.cpp -o Release/makebad