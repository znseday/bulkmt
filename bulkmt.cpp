#include <iostream>
#include <cassert>
#include <fstream>
#include <chrono>
#include <thread>

#include "bulkmt.h"

using namespace std;

std::mutex console_m;

/*
void TestFile(const char *file_name)
{
    cout << "File: " << file_name << endl;

    ifstream i_stream = ifstream(file_name);
    if (!i_stream)
    {
        cout << "My error: the input file not found" << endl;
        exit(0);
    }

    auto cmds = make_unique<Commands>(3);

    //LocalFileObserver LocalFileObs(&cmds);
    //ConsoleObserver   ConsoleObs(&cmds);

    auto ConsoleObs = make_shared<ConsoleObserver>();
    ConsoleObs->Register(cmds);

    auto LocalFileObs = std::make_shared<LocalFileObserver>();
    LocalFileObs->Register(cmds);

    string line;
    while (getline(i_stream, line))
    {
        std::this_thread::sleep_for(0.6s);
        cout << line << endl; // just echo

        cmds->AnalyzeCommand(line);
    }    
    cmds->ExecForAllSubs(true);

    i_stream.close();
    cout << endl << endl;
}
*/

