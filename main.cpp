#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <thread>

#include "math_f.h"
#include "bulkmt.h"

using namespace std;

int main(int argc, const char **argv)
{
    #if (defined WIN32) || (defined WIN64)
        cout << MY_P_FUNC << endl;                  // for debugging
        ifstream i_stream = ifstream("my_own_test.txt");
        if (!i_stream)
        {
            cout << "My error: the input file not found" << endl;
            exit(0);
        }
        //cerr << fi(30) << endl; // for debugging
    #else
        istream &i_stream = cin;
    #endif

    MY_DEBUG_ONLY(cout << "Homework bulkmt (DEBUG detected)" << endl);

    #if (defined WIN32) || (defined WIN64)
        //cout << "Tests on local machine:" << endl;
        //TestFile("test1.txt");
        //TestFile("test2.txt");
        //TestFile("test3.txt");
        //TestFile("test4.txt");
    #else
        // some
    #endif

    auto cmds_Console = make_unique<Commands>( (argc<2) ? 3 : static_cast<size_t>(atoi(argv[1])) );
    auto cmds_Files   = make_unique<Commands>( (argc<2) ? 3 : static_cast<size_t>(atoi(argv[1])) );

//    LocalFileObserver LocalFileObs(&cmds);
//    ConsoleObserver   ConsoleObs(&cmds);

    auto ConsoleObs = make_shared<ConsoleObserver>();
    ConsoleObs->Register(cmds_Console);

    auto LocalFileObs1 = std::make_shared<LocalFileObserver>();
    LocalFileObs1->Register(cmds_Files);

    auto LocalFileObs2 = std::make_shared<LocalFileObserver>();
    LocalFileObs2->Register(cmds_Files);

    string line;
    while (getline(i_stream, line))
    {
        #if (defined WIN32) || (defined WIN64)
            //std::this_thread::sleep_for(1.0s);
            console_m.lock();
            cout << line << endl; // just echo
            console_m.unlock();
        #else
            // nothing
        #endif

        cmds_Console->AnalyzeCommand(line);
        cmds_Files->AnalyzeCommand(line);
    }
    cmds_Console->ExecForAllSubs(true);
    cmds_Files->ExecForAllSubs(true);

    this_thread::sleep_for(1s); // for debugging

    ConsoleObs->Quit();
    LocalFileObs1->Quit();
    LocalFileObs2->Quit();

    ConsoleObs->Join();
    LocalFileObs1->Join();
    LocalFileObs2->Join();

    return 0;
}

