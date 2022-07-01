g++ --version

mkdir Release
g++ -std=c++14 -D_NDEBUG -O3 -lpthread -g bigsort.cpp -o Release\bigsort.exe
g++ -std=c++14 -D_NDEBUG -O3 -lpthread -g bigsort_test.cpp -o Release\bigsort_test.exe
g++ -std=c++14 -D_NDEBUG -O3 -lpthread -g makebad.cpp -o Release\makebad.exe

pause