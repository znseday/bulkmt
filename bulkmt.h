#pragma once

#include <iostream>
#include <fstream>

#include <vector>
#include <iterator>
#include <algorithm>
#include <memory>
//#include <chrono>
//#include <ratio>
//#include <thread>
#include <ctime>


using namespace std;

// __FUNCSIG__ is for VS, but Qt (mingw) works with __PRETTY_FUNCTION__
#if ((defined WIN32) || (defined WIN64)) && (defined _MSC_VER)
//#define MY_P_FUNC __FUNCSIG__
#else
#define MY_P_FUNC __PRETTY_FUNCTION__
#endif

#if ((defined NDEBUG) || (defined _NDEBUG))
#define MY_DEBUG_ONLY(x)
#else
#define MY_DEBUG_ONLY(x) (x)
#endif

void TestFile(const char *file_name);
//-----------------------------------------------

class Observer
{
private:

public:

    virtual void Do(/*OutputType out_, */const std::vector<std::string> &cmds, /*std::chrono::time_point<std::chrono::high_resolution_clock>*/ time_t t) = 0;
    virtual ~Observer() = default;
};
//-----------------------------------------------

class Commands
{
private:

    //std::vector<Observer *> subs;
    //std::vector<unique_ptr<Observer>> subs;
    std::vector<shared_ptr<Observer>> subs;

    size_t N = 3;
    size_t BracketOpenLevel = 0;
    std::vector<std::string> cmds;

    //std::chrono::time_point<std::chrono::high_resolution_clock> timeFirst;
    time_t timeFirst;

public:

    Commands(size_t _N) : N(_N) {}

    //void subscribe(Observer *obs)
    //void subscribe(shared_ptr<Observer> &&obs)
    void subscribe(const shared_ptr<Observer> &obs)
    {
        //subs.push_back(std::move(obs));
        subs.push_back(obs);
    }

    void AnalyzeCommand(const std::string &str)
    {
        if (str != "{" && str != "}")
        {
            if (cmds.empty())
                timeFirst = std::time(nullptr);
                //timeFirst = std::chrono::high_resolution_clock::now();

            cmds.push_back(str); // emplace_back
        }
        if (cmds.size() == N && !BracketOpenLevel)
        {
            ExecForAllSubs(false);
            //cmds.clear();
        }
        if (str == "{")
        {
            BracketOpenLevel++;

            if (BracketOpenLevel == 1 && !cmds.empty())
                ExecForAllSubs(false);

        }
        if (str == "}")
        {
            BracketOpenLevel--;
            if (BracketOpenLevel == 0 && !cmds.empty())
            {
                ExecForAllSubs(false);
                //cmds.clear();
            }
        }

    }

    void ExecForAllSubs(bool isFinished  /*const std::vector<std::string> &_cmds*/)
    {
        if ( !cmds.empty() &&  ( BracketOpenLevel == 0 || (BracketOpenLevel == 1 && !isFinished) ) )
        {
            for (auto &s : subs)
            {
                s->Do(cmds, timeFirst);
                //s.get()->Do(cmds, timeFirst);
            }

            cmds.clear();
        }
    }

};
//-----------------------------------------------

class ConsoleObserver : public std::enable_shared_from_this<ConsoleObserver>, public Observer
{
public:

//    ConsoleObserver(Commands *_cmds)
//    {
//    }

    //void JustNotConstructor(Commands *_cmds)
    void JustNotConstructor(const unique_ptr<Commands> &_cmds)
    {
        auto t = shared_from_this();
        _cmds->subscribe(t);
    }

    void Do(const std::vector<std::string> &cmds, [[maybe_unused]]/* std::chrono::time_point<std::chrono::high_resolution_clock>*/ time_t t) override
    {
        //std::cout << MY_P_FUNC << std::endl;
        std::cout << "bulk: ";
        size_t cmds_size = cmds.size();
        for (size_t i = 0; i < cmds_size; i++)
            std::cout << cmds[i] << (  (i<(cmds_size-1)) ? ", " : "\n");
    }
};
//-----------------------------------------------

class LocalFileObserver : public Observer, public std::enable_shared_from_this<LocalFileObserver>
{
public:
//    LocalFileObserver(Commands *_cmds)
//    {
//    }

    //void JustNotConstructor(Commands *_cmds)
    void JustNotConstructor(const unique_ptr<Commands> &_cmds)
    {
        _cmds->subscribe(shared_from_this());
    }

    void Do(const std::vector<std::string> &cmds, /*std::chrono::time_point<std::chrono::high_resolution_clock>*/ time_t t) override
    {
        //std::cout << MY_P_FUNC << std::endl;
        //size_t tInSecs = std::chrono::duration_cast<std::chrono::milliseconds>(t).count(); // doesn't work. Why ???

        ofstream f( std::string("bulk") + std::to_string(t) + std::string(".log") );

        size_t cmds_size = cmds.size();
        for (size_t i = 0; i < cmds_size; i++)
            f << cmds[i] << std::endl;

        f.close();
    }
};
//-----------------------------------------------







