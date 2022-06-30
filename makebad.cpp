/*
Utility for writing ordered sequences to file.
Copyright 2022 Yuriy Fadeyev
License: MIT

*/

#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>


using namespace std;

#define DATATYPE uint32_t

int main(int argc, char *argv[])
{
    int excode = -1;
    string fname = "input";
    uint64_t ifsize = 5000;
    bool good = false;

    if (argc > 1)
        fname = argv[1];
    if (argc > 2)
      ifsize = stoul(argv[2]);
    if (argc > 3)
        good = true;

    ofstream ofs;
    ofs.open(fname, ios::binary | ios::out | ios::trunc);
    if (!ofs.is_open())
    {
        cerr << "Error opening file: '" << fname <<"'.\n";
    }
    else
    {
        DATATYPE val = 0;
        if (good)
        {
            for (uint64_t i = 0; i < ifsize / sizeof(DATATYPE); i++)
            {
                val = (DATATYPE) i;
                ofs.write(reinterpret_cast<char*>(&val), sizeof(DATATYPE));
            }

        }
        else
        {
            for (uint64_t i = ifsize / sizeof(DATATYPE); i > 0; i--)
            {
                val = (DATATYPE) i;
                ofs.write(reinterpret_cast<char*>(&val), sizeof(DATATYPE));
            }

        }

        ofs.close();        

        excode = 0;

    }

    cout << endl;
    return excode;

}