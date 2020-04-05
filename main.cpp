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

constexpr auto DEF_N_BLOCKS = 2;


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

    auto cmds_Console = make_unique<CommandsHandler>( (argc<2) ? DEF_N_BLOCKS : static_cast<size_t>(atoi(argv[1])) );
    auto cmds_Files   = make_unique<CommandsHandler>( (argc<2) ? DEF_N_BLOCKS : static_cast<size_t>(atoi(argv[1])) );

    auto ConsoleObs = make_shared<ConsoleObserver>(cmds_Console->to_push_mutex);
    ConsoleObs->Register(cmds_Console);

    auto LocalFileObs1 = std::make_shared<LocalFileObserver>(cmds_Files->to_push_mutex);
    LocalFileObs1->Register(cmds_Files);

    auto LocalFileObs2 = std::make_shared<LocalFileObserver>(cmds_Files->to_push_mutex);
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

    ConsoleObs->Quit();
    LocalFileObs1->Quit();
    LocalFileObs2->Quit();

    ConsoleObs->NotifyAll();
    LocalFileObs1->NotifyAll();
    LocalFileObs2->NotifyAll();

    ConsoleObs->Join();
    LocalFileObs1->Join();
    LocalFileObs2->Join();

    cout << "Ok, it's done" << endl;

    cout << "Main thread: ";    cmds_Files->PrintMetrics(); cout << endl;
    cout << "Console thread: "; ConsoleObs->PrintMetrics(); cout << endl;
    cout << "File 1  thread: "; LocalFileObs1->PrintMetrics(); cout << endl;
    cout << "File 2  thread: "; LocalFileObs2->PrintMetrics(); cout << endl;

    return 0;
}

