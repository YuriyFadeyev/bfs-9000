/*
Utility to sort values in large files (ascending order). Just for fun.
Copyright 2022 Yuriy Fadeyev
License: MIT
*/

#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <string>
#include <algorithm>    // std::sort
#include <chrono>
#include <ctime>
#include <typeinfo>

using namespace std;

#define MEM_LIMIT 64000000
#define MEM_LIMIT_MIN 1000
#define MEM_LIMIT_MAX 120000000

// treat values as this type:
#define DATATYPE uint32_t

// <filesystem> is available since C++17, so here's some workaround
#if defined _WIN32 || defined _WIN64 || defined __CYGWIN__
    #define _WINDOWS_
    #include "fileapi.h"
    uint64_t GetFreeSpace()
    {
        ULARGE_INTEGER lFreeBytesAvailableToCaller;
        //ULARGE_INTEGER lTotalNumberOfBytes;
        //ULARGE_INTEGER lTotalNumberOfFreeBytes;

        if (! GetDiskFreeSpaceExA(nullptr, &lFreeBytesAvailableToCaller, nullptr, nullptr)) 
        {
            cerr << "Can't retrieve disk free space info." << std::endl;
            return 0ull;
        }

        return lFreeBytesAvailableToCaller.QuadPart;
    }
#elif defined __linux__ || defined __unix__ ||  defined __FreeBSD__ || defined __APPLE__
    #define _NIX_
    #include <sys/statvfs.h>

    // maximum amount of file streams. 
    #ifdef __APPLE__
        #define MAXSTREAMS 256
    #else
        #define MAXSTREAMS 1024
    #endif

    uint64_t GetFreeSpace()
    {
        struct statvfs fiData;

        if((statvfs(".", &fiData)) < 0 ) 
        {
            cerr << "Error calling statvfs()\n";
            return 0;
        }
        else
            return fiData.f_bavail * fiData.f_bsize;
    }

    // no implementation for *nix yet
    int _getmaxstdio( void )
    {
        return MAXSTREAMS; // should work on most systems
    }

    // no implementation for *nix yet
    int _setmaxstdio(int new_max)
    {
        return -1;
    };
#else
    #error Unknown OS.
#endif


/*
 * Helper class for writing and reading tempfiles 
*/
class MyTempFile
{
private:
    fstream fs;
public:
    //name of tempfile. Normally should not be accessed from outside.
    string fname;
    //value on the head of tempfile. Must not be written from outside.
    DATATYPE val;

    // Constructor. Makes tempfile name.
    MyTempFile(size_t idx)
    {
        fname = "temp" + to_string(idx);
    }

    // Destructor. Closes tempfile and removes it from disk
    ~MyTempFile()
    {
        if (fs.is_open())
            fs.close();

        if (remove(fname.c_str()) != 0)
            cerr << "\nError deleting tempfile '" << fname << "'\n";

    }

    // opens temp file, writes data to it
    // @param s: pointer to buffer
    // @param n: bytes to write
    // @returns true if success, false if error
    bool write(const char* s, streamsize n)
    {
        fs.open(fname, fstream::binary | fstream::in | fstream::out | fstream::trunc);
        if (!fs.is_open())
        {
            cerr << "\nError opening temp file '" << fname << "'\n";
            return false;
        }
        fs.write(s, n);
        if (fs.fail())
        {
            cerr << "\nError writing temp file '" << fname << "'\n";
            return false;
        }
        fs.seekg(0, fs.beg);

        return peek_val();

    }

    // peeks next value from tempfile stream and puts it to this.val
    // @returns false if no value available or read error occured
    bool peek_val()
    {
        DATATYPE t;
        fs.read(reinterpret_cast<char*>(&t), sizeof(t));
        if (fs.gcount() == sizeof(t))
        {
            val = t;
            return true;
        }
        //cerr << "Error reading data from temp file" << endl;
        return false;
    }
};

//*****************************************************************************
// program entry point
//*****************************************************************************
int main(int argc, char *argv[])
{
    int excode = -1;

    // greetings
    cout << "Big File Sorter (BFS-9000) v0.1 (" << __DATE__ << " " << __TIME__ ").\n"
        << "(c) 2022 Yuriy Fadeyev. This is free software; License MIT. \n\n";

    string ifname = "input";
    string ofname = "output";
    uint32_t mem_limit = MEM_LIMIT;

    // parsing optional arguments. The order is important.
    if (argc == 1)
    {
        // no params given, print help
        cout << "Usage: bigsort [infile] [outfile] [mem_limit]\n"
            << "[infile]\t- input file. Default = 'input'\n"
            << "[outfile]\t- ouput file. Default = 'output'\n"
            << "[mem_limit]\t- size of buffer for reading, bytes.\n"
            <<  "\t\t  " << MEM_LIMIT_MIN << "..." << MEM_LIMIT_MAX << ", Default = " << MEM_LIMIT << "\n"
            << "Order of optional parameters is IMPORTANT.\n\n\n";
    }
    else if (argc > 1)
        ifname = argv[1];
    if (argc > 2)
        ofname = argv[2];
    if (argc > 3)
    {
        try
        {
            mem_limit = stoi(argv[3]);
            if (mem_limit < MEM_LIMIT_MIN || mem_limit > MEM_LIMIT_MAX)
                throw std::range_error("value is out of allowed range.");
            if (mem_limit % sizeof(DATATYPE) != 0)
                throw std::runtime_error("value must be a multiple of data size");
        }
        catch(const std::exception& e)
        {
            std::cerr << "bad mem_limit (" << e.what() << ")\n";
            return -2;
        }
    }

    // print actual parameters
    cout << "Run with parameters:\n"
        << "infile  \t'" << ifname << "'\n"
        << "outfile  \t'" << ofname << "'\n"
        << "mem_limit \t" << mem_limit << "\n"
        << "data type \t" << typeid(DATATYPE).name() << "\n"
        << "data size \t" << sizeof(DATATYPE) 
        << endl;

    // begin timing
    auto t_start = std::chrono::high_resolution_clock::now();

    ifstream ifs;
    ofstream ofs;
    // dynamic list (on stack) of pointers to MyTempFile helper class insances (on heap),
    vector<MyTempFile*> tempfiles;

    try
    {
        // main idea: read input file into buf, block after block. sort each block and write it to tempfile
        // then merge all temp files into output file

        // input buffer (on heap)
        std::vector<DATATYPE> buf(mem_limit / sizeof(DATATYPE));

        // open and examine input file
        ifs.open(ifname, ios::binary);
        if (!ifs.is_open())
        {
            cerr << "Error opening input file: '" << ifname <<"'.";
            throw runtime_error("read error");
        }

        // check input file size
        auto ifoffs = ifs.tellg();
        ifs.seekg(0, ios::end);
        int64_t ifsize = (int64_t) ifs.tellg() - ifoffs; 

        if (ifsize < (long int) sizeof(DATATYPE))
        {
            cerr << "File too small: '" << ifname <<"'.";
            throw runtime_error("read error");
        }

        cout << "\nPrepare workspace:\n"
            << "ifsize, bytes \t" << ifsize << "\n"
            << "values to read \t" << ifsize / sizeof(DATATYPE) 
            << endl;

        if (ifsize % sizeof(DATATYPE) != 0)
            cout << "! file size is not a multiple of the data type size, last bytes will be dropped\n";

        // how many blocks do we need?
        auto block_cnt = (ifsize-1l) / mem_limit + 1;
        cout << "need block_cnt\t" << block_cnt << endl;

        // check if we have enough fstreams:
        // depends on C runtime library implementation
        // default 512 in Windows, may be increased up to 2048 (or even 8192?)
        // 1024 in *nix

        // _getmaxstdio(), _setmaxstdio()  are defineed in <stdio.h> for Windows
        // for *nix we use cour crutch-styled definitions

        auto stream_cnt = block_cnt + 8; //+ a little extra for stdin, stdout, stderr, etc.

        auto stream_avail = _getmaxstdio();
        if(stream_cnt > stream_avail)
        {
            // let's try to increase file stream limit
            cout << "_getmaxstdio() = " << stream_avail << ", trying to increase...\n";
            if (_setmaxstdio(stream_cnt) < 0)
            {
                cerr << "Error increasing i/o streams limit\n";
            };
        }

        // increasing the limit had no success 
        stream_avail = _getmaxstdio();
        if (stream_cnt > stream_avail)
        {
            cerr << "Not enough i/o streams (available " << stream_avail << ", required " << stream_cnt 
                << "). Try to increase mem_limit parameter.\n";
            throw runtime_error("_setmaxstdio()");
        }


        cout << "_getmaxstdio() \t" << stream_avail << endl; 

        // check if we have enough space for output and temporary files
        int64_t freespace = GetFreeSpace();
        cout << "Free space, b \t" << freespace << endl;

        // we need at least 2x bytes for writing and some reserve
        if (freespace < ifsize * 2 + 10000000)
        {
            cerr << "Not enough disk spase (at least 2x input size required)\n";
            throw std::runtime_error("Not enough disk space.");
        }

        // open output file for writing
        ofs.open(ofname, fstream::binary | fstream::in | fstream::out | fstream::trunc);
        if (!ofs.is_open())
        {
            cerr << "Error opening output file: '" << ofname <<"'.";
            throw runtime_error("write error");
        }


        // let's start slicing our in file
        ifs.seekg(0, ios::beg);
        cout << endl;

        auto t_prep = std::chrono::high_resolution_clock::now();
        cout << "t_prep, ms \t" << std::chrono::duration<double, std::milli>(t_prep-t_start).count() << endl;

        cout << "\nSlice big file to " << block_cnt << " temp files:\n";

        uint32_t blocks_done = 0;
        uint64_t values_done = 0;

        // accumulated timing:
        // input reading time
        auto t_read = std::chrono::duration<double, std::milli>(0).count();;
        // sorting time
        auto t_sort = t_read;
        // tempfile writing time
        auto t_write = t_read;

        while (!ifs.eof())
        {
            auto t_read_start = std::chrono::high_resolution_clock::now();

            // read block from input file
            ifs.read(reinterpret_cast<char*>(buf.data()), mem_limit);

            auto t_read_end = std::chrono::high_resolution_clock::now();
            t_read += std::chrono::duration<double, std::milli>(t_read_end-t_read_start).count();

            auto values_read = ifs.gcount() / sizeof(DATATYPE);

            if(values_read > 0)
            {
                // sort block in memory
                sort(buf.begin(), buf.begin() + values_read);

                auto t_sort_end = std::chrono::high_resolution_clock::now();
                t_sort += std::chrono::duration<double, std::milli>(t_sort_end-t_read_end).count();

                // write block to temp  file using helper class
                // create dynamic instance of MyTempFile on heap
                MyTempFile *tf = new MyTempFile(tempfiles.size());
                if (tf->write(reinterpret_cast<char*>(buf.data()), values_read * sizeof(DATATYPE)))
                {
                    tempfiles.push_back(tf);
                    blocks_done++;
                    values_done += values_read;
                    cout << ".";
                }
                else
                {
                    cerr << "\nError writing block " << blocks_done << " of " << block_cnt << "\n";
                    // this class object is not destroyed automatically, so let's do it here
                    delete tf;
                    throw runtime_error("tempfile error");
                }

                auto t_write_end = std::chrono::high_resolution_clock::now();
                t_write += std::chrono::duration<double, std::milli>(t_write_end-t_sort_end).count();
            }
        
        }
        ifs.close();        
        cout << endl;

        auto t_end1 = std::chrono::high_resolution_clock::now();

        cout << "t_slice, ms \t" << std::chrono::duration<double, std::milli>(t_end1-t_start).count() << endl;
        cout << "    t_read, ms \t" << t_read << endl;
        cout << "    t_sort, ms \t" << t_sort << endl;
        cout << "   t_write, ms \t" << t_write << endl;

        cout << "\nMerge " << tempfiles.size() << " tempfiles: \n";

        // merge values from set of tempfiles to output file

        // pointer to cur position in our i/o buffer
        auto pbuf = buf.begin();

        while (tempfiles.size())
        {
            // find the tempfile with minimum value at its head
            auto me = std::min_element(tempfiles.begin(), tempfiles.end(), [](auto a, auto b) {return a->val < b->val;});

            // write this value to i/o buffer and move the pointer
            *pbuf = (*me)->val;
            pbuf++;
            // if the buffer is full, write it to disk and rewind pointer
            if (pbuf == buf.end())
            {
                pbuf = buf.begin();
                ofs.write(reinterpret_cast<char*>(buf.data()), mem_limit);
                cout << ".";
            }

            // peek next value from current tempfile
            if (!(*me)->peek_val())
            {
                // erase() does not free *object, let's do it manually 
                delete *me;
                tempfiles.erase(me);
            }


        }

        // write the rest of the data
        ofs.write(reinterpret_cast<char*>(buf.data()), (pbuf - buf.begin()) * sizeof(DATATYPE));
        ofs.close();
        cout << ".\n\n";

        auto t_end2 = std::chrono::high_resolution_clock::now();
        cout << "t_merge, ms \t" << std::chrono::duration<double, std::milli>(t_end2-t_end1).count() << endl;

        excode = 0;
    }
    catch(const std::exception& e)
    {
        std::cerr << "\nSomething went wrong. Guess what? (" << e.what() << ")\n";
        excode = -10000;
    }

    // try some cleanup
    try
    {
        cout << "\nClean up:\n";
        // close files (yes, they will be closed in corresponding destructor, but ...)
        if (ifs.is_open())
            ifs.close();
        if (ofs.is_open())
            ofs.close();

        // objects in the list are not disposed automatically, let's kill'em all
        for (auto it: tempfiles)
        {
            cout << ".";
            delete it;
            excode--; //if some tempfiles were not closed automatically - it may be a bad sympthom 
        }

        cout << "\ndone.\n";
    }
    catch(const std::exception& e)
    {
        std::cerr << "\nSomething went wrong. Guess what? (" << e.what() << ")\n";
        excode = -20000;
    }


    cout << "\nTotal time, ms\t" << std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now()-t_start).count() << endl;
    
    return excode;
}