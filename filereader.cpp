#include "filereader.h"
#include "flvparser.h"

#include <fstream>
#include <iostream>
#include <thread>
#include <memory>

using namespace std;

thread * file_load_thread = nullptr;

namespace {

const int MAX_BUF_LEN = 4096;

}

void load_file_loop(shared_ptr<ifstream> ifs_ptr)
{
    ifstream & ifs = *ifs_ptr;

    char c[MAX_BUF_LEN] = { 0 };

    // "FLV"
    ifs.read(c, 3);
    // Version
    ifs.read(c, 1);
    // TypeFlags
    ifs.read(c, 1);
    // DataOffset
    ifs.read(c, 4);

    for (int i = 0; true; i++)
    {
        bool pasre_ret = parse_body(ifs);

        if (!pasre_ret)
        {
            cout << "all parse over" << endl;
            break;
        }
    }
}

int load_file(const std::string & filename)
{
    shared_ptr<ifstream> ifs_ptr(new ifstream(filename, ios_base::in | ios_base::binary));
    ifstream & ifs = *ifs_ptr;

    if (!ifs.is_open())
    {
        cerr << " can not open file " << filename << endl;

        return -1;
    }

    file_load_thread = new thread(std::bind(load_file_loop, ifs_ptr));

    return 0;
}


