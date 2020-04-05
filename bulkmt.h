#pragma once

#include <iostream>
#include <fstream>

#include <vector>
#include <iterator>
#include <algorithm>
#include <memory>
#include <thread>
#include <ctime>
#include <sstream>
#include <condition_variable>
#include <iostream>
#include <queue>
#include <thread>
#include <future>

#include "math_f.h"
#include "metrics.h"

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

using CommandsType = std::vector<std::string>;
using CommandBlocksType = std::queue<CommandsType>;

using args = std::tuple<
        std::function<void(CommandsType)>,
        CommandBlocksType &
        >;

//-----------------------------------------------

class Observer
{
protected:

    std::condition_variable cv;

    std::mutex &p_cv_m;

    std::queue<args> q_functions; // tuple of std::functions and CommandBlocksType
    std::atomic_bool quit = false;

    MetricsLocalThread MetricsLocal;

    std::thread thread;


public:

    /*
    void operator()()
    {
        while (!(quit && q_functions.empty()))
        {
            //if (!p_cv_m)
            //    return;

            std::unique_lock<std::mutex> lk(p_cv_m);

            //console_m.lock();
            //std::cerr << std::this_thread::get_id() << " waiting... " << std::endl;
            //console_m.unlock();

            //cv.wait(lk, [&msgs]() { return !msgs.empty() || quit; });

            cv.wait(lk, [this]() { return !q_functions.empty() || quit; });

            if (!q_functions.empty())
            {
                auto [f, blocks] = std::move(q_functions.front());
                q_functions.pop();

                CommandsType block;

                if (!blocks.empty())
                {
                    block = blocks.front();
                   // block = std::move(blocks.front());
                    blocks.pop();

                    MetricsLocal.nBlocks++;
                    MetricsLocal.nCommands += block.size();
                }

                //auto s = q_functions.size();
                lk.unlock();

//               console_m.lock();
//               std::cerr << std::this_thread::get_id() << " s = " << s << "   before f.get()" << std::endl;
//               console_m.unlock();

                f(block);

                //f.get();

                //console_m.lock();
                //std::cerr << std::this_thread::get_id() << " leave " << s << std::endl;
                //console_m.unlock();
           }
        }
    }
    */


    // сначала хотел просто сделать Observer() : thread(this) и описать operator(), т.е. сделать this callable-объектом. Но почему-то не компилировалось.
    // придумал вариант с лямбдой в конструкторе, в этом варианте работает.

    Observer(std::mutex &_p_cv_m) : p_cv_m(_p_cv_m), //thread(*this)
      thread( [this](){ while (!(quit && q_functions.empty())) // chanched for "graceful shutdown"
     //{}

        {
            std::unique_lock<std::mutex> lk(p_cv_m);

            cv.wait(lk, [this]() { return !q_functions.empty() || quit; });

            if (!q_functions.empty())
            {
                auto [f, blocks] = std::move(q_functions.front());
                q_functions.pop();

                CommandsType block;

                if (!blocks.empty())
                {
                    block = blocks.front();
                    //block = std::move(blocks.front());
                    blocks.pop();

                    MetricsLocal.nBlocks++;
                    MetricsLocal.nCommands += block.size();
                }

                lk.unlock();

                f(block);
           }
        }}  )
    {}


    virtual void Do(/*const*/ CommandBlocksType &cmds_blocks, time_t t) = 0;

    virtual ~Observer() {quit = true;}

    void NotifyAll() {cv.notify_all();}
    void Join() {thread.join();}
    void Quit() {quit = true;}

    void PrintMetrics() { cout << MetricsLocal;}
};
//-----------------------------------------------

class CommandsHandler
{
private:

    std::vector<shared_ptr<Observer>> subs;

    size_t N = 3;
    size_t BracketOpenLevel = 0;
    CommandsType cmds;

    time_t timeFirst;

    CommandBlocksType CommandBlocks;

    MetricsMainThread MetricsMain;

public:

    std::mutex to_push_mutex;

    CommandsHandler(size_t _N) : N(_N) {}

    void subscribe(const shared_ptr<Observer> &obs)
    {
        subs.push_back(obs);
    }

    void AnalyzeCommand(const std::string &str)
    {
        MetricsMain.nLines++;

        if (str != "{" && str != "}")
        {
            if (cmds.empty())
                timeFirst = std::time(nullptr);

            cmds.push_back(str); // emplace_back
        }
        if (cmds.size() == N && !BracketOpenLevel)
        {
            ExecForAllSubs(false);
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
            to_push_mutex.lock();
            CommandBlocks.push(cmds);
            to_push_mutex.unlock();

            MetricsMain.nBlocks++;
            MetricsMain.nCommands += cmds.size();

            for (auto &s : subs)
            {
                s->Do(CommandBlocks, timeFirst);   // "Do" adds a command into a queue
            }

            cmds.clear();
        }
    }

    void PrintMetrics() { cout << MetricsMain;}
};
//-----------------------------------------------

class ConsoleObserver : public std::enable_shared_from_this<ConsoleObserver>, public Observer
{
public:

    ConsoleObserver(std::mutex &_p_cv_m) : Observer(_p_cv_m) {}

    void Register(const unique_ptr<CommandsHandler> &_cmds)
    {
        _cmds->subscribe(shared_from_this());
    }

    void Do(/*const*/ CommandBlocksType &cmds_blocks, [[maybe_unused]] time_t t) override
    {
        {
            std::lock_guard<std::mutex> lk(p_cv_m);

            q_functions.emplace( [](CommandsType _block)
                {                   
                    if (_block.empty())
                        return;

                    stringstream s;
                    s << "bulk: ";

                    size_t cmds_size = _block.size();

                    s << "(size = " << cmds_size << ") : ";

                    for (size_t i = 0; i < cmds_size; i++)
                    {
                        unsigned long long fa_res = fa(stoi(_block[i]));
                        s << fa_res << (  (i<(cmds_size-1)) ? ", " : "\n");
                    }

                    console_m.lock();
                    std::cout << s.str();
                    console_m.unlock();

                } // end of lambda
                , cmds_blocks);
        }
        cv.notify_one();

    }
};
//-----------------------------------------------

class LocalFileObserver : public Observer, public std::enable_shared_from_this<LocalFileObserver>
{
public:

    LocalFileObserver(std::mutex &_p_cv_m) : Observer(_p_cv_m) {}

    void Register(const unique_ptr<CommandsHandler> &_cmds)
    {
        _cmds->subscribe(shared_from_this());
    }

    void Do(/*const*/ CommandBlocksType &cmds_blocks, time_t t) override
    {    
        {
            std::lock_guard<std::mutex> lk(p_cv_m);

            q_functions.emplace( [t, this](CommandsType _block)
                {               
                    if (_block.empty())
                        return;

                    static std::atomic_int nFile = 0; // atomic???

                    stringstream s;
                    s << "bulk" << t << "-" << std::this_thread::get_id() << "-" << nFile++ << ".log";
                    ofstream f( s.str() );

                    size_t cmds_block_size = _block.size();

                    for (size_t i = 0; i < cmds_block_size; i++)
                    {
                        unsigned long long fi_param = stoi(_block[i]);

                        unsigned long long fi_res = fi(fi_param);

                        f << fi_res << std::endl;
                    }

                    f.close();

                },  // end of lambda
                cmds_blocks);
        }
        cv.notify_one();
    }
};
//-----------------------------------------------




