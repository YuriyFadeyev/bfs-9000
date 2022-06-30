/*
Utility to check if values in large files are in ascending order.
Copyright 2022 Yuriy Fadeyev
License: MIT
*/
#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <string>
#include <algorithm>    // std::sort

using namespace std;

#define DATATYPE uint32_t


int main(int argc, char *argv[])
{
    int excode = -1;
    string ifname = "output";

    if (argc > 1)
        ifname = argv[1];



    ifstream ifs;
    ifs.open(ifname, ios::binary);
    if (!ifs.is_open())
    {
        cerr << "Error opening file: '" << ifname <<"'.";
    }
    else
    {
        DATATYPE val = 0, new_val;
        uint64_t idx = 0, bad_idx = 0;
        while (!ifs.eof())
        {
            ifs.read(reinterpret_cast<char*>(&new_val), sizeof(DATATYPE));
            if (ifs.gcount() == sizeof(DATATYPE))
            {
                if ((new_val < val) && (0 != idx))
                {
                    bad_idx = idx;
                    break;
                }
                else
                {
                    val = new_val;
                    idx++;
                }
            }
        }

        ifs.close();        

        if (bad_idx > 0)
        {
            cout << "Bad value at position " << bad_idx << endl;
            excode = bad_idx;
        }
        else
        {
            cout << "File is sorted ascending. Values found: " << idx << endl;
            excode = 0;
        }

    }

    return excode;

}