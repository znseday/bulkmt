#pragma once

#include <iostream>
#include <fstream>

#include <vector>
#include <iterator>
#include <algorithm>
#include <memory>
//#include <chrono>
//#include <ratio>
#include <thread>
#include <ctime>
#include <sstream>
#include <condition_variable>
#include <iostream>
#include <queue>
#include <thread>
#include <future>

#include "math_f.h"

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

extern std::mutex console_m;

//using args = std::tuple<
////        std::function<void(const std::string &, const std::string &)>,
//        std::future<void>
//        >;

using args = std::future<void>;

//-----------------------------------------------


class Observer
{
protected:

    std::condition_variable cv;
    std::mutex cv_m;

    std::thread thread;

    std::queue<args> msgs; // futures!
    std::atomic_bool quit = false;

//    void operator()()
//    {
//        while (!quit)
//        {
//            std::unique_lock<std::mutex> lk(cv_m);
//            console_m.lock();
//            std::cerr << std::this_thread::get_id() << " waiting... " << std::endl;
//            console_m.unlock();
//            //cv.wait(lk, [&msgs]() { return !msgs.empty() || quit; });
//            cv.wait(lk, [this]() { return !msgs.empty() || quit; });

//            if (!msgs.empty())
//            {
//               auto [f] = std::move(msgs.front());
//               msgs.pop();

//               auto s = msgs.size();
//               lk.unlock();

//               f.get();

//               console_m.lock();
//               std::cerr << std::this_thread::get_id() << " leave " << s << std::endl;
//               console_m.unlock();
//           }
//        }
//    }

public:

    Observer() : thread( [this](){ while (!quit)
        {
            std::unique_lock<std::mutex> lk(cv_m);
            console_m.lock();
            std::cerr << std::this_thread::get_id() << " waiting... " << std::endl;
            console_m.unlock();
            //cv.wait(lk, [&msgs]() { return !msgs.empty() || quit; });
            cv.wait(lk, [this]() { return !msgs.empty() || quit; });

            if (!msgs.empty())
            {
               auto f = std::move(msgs.front());
               msgs.pop();

               auto s = msgs.size();
               lk.unlock();

//               console_m.lock();
//               std::cerr << std::this_thread::get_id() << " s = " << s << "   before f.get()" << std::endl;
//               console_m.unlock();

               f.get();

               console_m.lock();
               std::cerr << std::this_thread::get_id() << " leave " << s << std::endl;
               console_m.unlock();
           }
        }}  )
    {}

    virtual void Do(/*OutputType out_, */const std::vector<std::string> &cmds, time_t t) = 0;
    virtual ~Observer() {quit = true;}

    void Join() {thread.join();}
    void Quit() {quit = true;}
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

    void ExecForAllSubs(bool isFinished)
    {      
        if ( !cmds.empty() &&  ( BracketOpenLevel == 0 || (BracketOpenLevel == 1 && !isFinished) ) )
        {
            for (auto &s : subs)
            {
                s->Do(cmds, timeFirst);             // "Do" means to add into queue of futures !
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

    void Register(const unique_ptr<Commands> &_cmds)
    {
        auto t = shared_from_this();
        _cmds->subscribe(t);
    }

    void Do(const std::vector<std::string> &cmds, [[maybe_unused]] time_t t) override
    {
        {
            std::lock_guard<std::mutex> lk(cv_m);
            msgs.emplace(std::async( std::launch::deferred, [cmds]()
                {
                    console_m.lock();
                    std::cout << "bulk: ";
                    size_t cmds_size = cmds.size();
                    std::cout << "(size = " << cmds_size << ") : ";
                    console_m.unlock();
                    //size_t cmds_size = cmds.size();
                    for (size_t i = 0; i < cmds_size; i++)
                    {
                        unsigned long long fa_res = fa(stoi(cmds[i]));
                        console_m.lock();
                        std::cout << fa_res << (  (i<(cmds_size-1)) ? ", " : "\n");
                        console_m.unlock();
                    }

                } ));

        }
        cv.notify_one();

//        std::cout << "bulk: ";
//        size_t cmds_size = cmds.size();
//        for (size_t i = 0; i < cmds_size; i++)
//            std::cout << fa(stoi(cmds[i])) << (  (i<(cmds_size-1)) ? ", " : "\n");
    }
};
//-----------------------------------------------

class LocalFileObserver : public Observer, public std::enable_shared_from_this<LocalFileObserver>
{
protected:
    //std::mutex file_m;
public:

    void Register(const unique_ptr<Commands> &_cmds)
    {
        _cmds->subscribe(shared_from_this());
    }

    void Do(const std::vector<std::string> &cmds, time_t t) override
    {    
        {
            std::lock_guard<std::mutex> lk(cv_m);
            msgs.emplace(std::async( std::launch::deferred, [cmds, t]()
                {
                    stringstream s;
                    s << "bulk" << t << "-" << std::this_thread::get_id() << ".log";
                    ofstream f( s.str() );

                    size_t cmds_size = cmds.size();

                    for (size_t i = 0; i < cmds_size; i++)
                    {
                        //file_m.lock();
                        unsigned long long fi_res = fi(stoi(cmds[i]));
                        //file_m.unlock();

                        f << fi_res << std::endl;
                    }

                    f.close();

                } ));

        }
        cv.notify_one();


//        stringstream s;
//        s << "bulk" << t << "-" << std::this_thread::get_id() << ".log";
//        ofstream f( s.str() );

//        size_t cmds_size = cmds.size();
//        for (size_t i = 0; i < cmds_size; i++)
//            f << fi(stoi(cmds[i])) << std::endl;

//        f.close();
    }
};
//-----------------------------------------------




