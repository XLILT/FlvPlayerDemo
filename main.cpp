#include "h264decode.h"
#include "aacdecode.h"
#include "rendercommon.h"
#include "filereader.h"

#include <iostream>
#include <bitset>
#include <assert.h>
#include <vector>

using namespace std;

int main(int argc, char ** argv)
{
    if (argc < 2)
    {
        cerr << "Usage:" << endl;
        cerr << "  " << argv[0] << " flvfile" << endl;
        
        getchar();

        exit(-1);
    }

    const char * filename = argv[1];
    
    int render_init_ret = render_init();
    assert(render_init_ret == 0);

    // init h264 decode global enviroment
    int avc_init_ret = h264_init();
    assert(avc_init_ret == 0);

    int aac_init_ret = aac_init();
    assert(aac_init_ret == 0);

    int file_init_ret = load_file(filename);
    assert(file_init_ret == 0);

    render_event_loop();
    
    aac_fina();
    h264_fina();
    render_fina();

    return 0;
}
