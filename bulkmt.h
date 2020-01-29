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

//using args = std::tuple<
////        std::function<void(const std::string &, const std::string &)>,
//        std::future<void>
//        >;


using CommandsType = std::vector<std::string>;
using CommandBlocksType = std::queue<CommandsType>;

//using args = std::function<void(CommandsType)>;

//using args = std::future<void(CommandBlocksType)>;

using args = std::tuple<
        std::function<void(CommandsType)>,
        CommandBlocksType &
        >;

//-----------------------------------------------


class Observer
{
protected:

    std::condition_variable cv;
    std::mutex cv_m;

    std::queue<args> q_functions; // tuple of std::functions and CommandBlocksType
    std::atomic_bool quit = false;

    MetricsLocalThread MetricsLocal;

    std::thread thread;

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

    // сначала хотел просто сделать Observer() : thread(this) и описать operator(), т.е. сделать this callable-объектом. Но почему-то не компилировалось.
    // придумал вариант с лямбдой в конструкторе, в этом варианте работает.
    // Также в голове вертится мысль "а можно ли отнаследоватся от thread и что-то переопределить волшебное?", т.к. именно таким способом принято программировать многопоточность в C++Builder с их библиотечными потоками.

    Observer() : thread( [this](){ while (!quit)
        {
            std::unique_lock<std::mutex> lk(cv_m);

            //console_m.lock();
            //std::cerr << std::this_thread::get_id() << " waiting... " << std::endl;
            //console_m.unlock();

            //cv.wait(lk, [&msgs]() { return !msgs.empty() || quit; });  // из лекции
            cv.wait(lk, [this]() { return !q_functions.empty() || quit; });

            if (!q_functions.empty())
            {
                auto [f, blocks] = std::move(q_functions.front());
                q_functions.pop();

                CommandsType block;

                if (!blocks.empty())
                {
                    block = /*std::move*/(blocks.front());
                    blocks.pop();

                    MetricsLocal.nBlocks++;
                    MetricsLocal.nCommands += block.size();
                }

                //auto s = q_functions.size();
                lk.unlock();

//               console_m.lock();
//               std::cerr << std::this_thread::get_id() << " s = " << s << "   before f.get()" << std::endl;
//               console_m.unlock();

                f(block);  // Выполняем function с параметром

                //f.get(); // Просто выполняем фьючу чтобы там ни лижало

                //console_m.lock();
                //std::cerr << std::this_thread::get_id() << " leave " << s << std::endl;
                //console_m.unlock();
           }
        }}  )
    {}

    virtual void Do(/*const*/ CommandBlocksType &cmds_blocks, time_t t) = 0;

    virtual ~Observer() {quit = true;} // на всякий случай. Это вообще хороая идея? Имеет ли смысл?

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

    std::mutex to_push_mutex;

    MetricsMainThread MetricsMain;

public:

    CommandsHandler(size_t _N) : N(_N) {}

    void subscribe(const shared_ptr<Observer> &obs)
    {
        //subs.push_back(std::move(obs));
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
            to_push_mutex.lock();      // Нужен ли этот мьютекс? Блочит ли он все остальные потоки?
            CommandBlocks.push(cmds);  // Нужно, чтобы этот push как-то выполнился атомарно
            to_push_mutex.unlock();

            MetricsMain.nBlocks++;
            MetricsMain.nCommands += cmds.size();

            for (auto &s : subs)
            {
                s->Do(CommandBlocks, timeFirst);   // "Do" здесь значит просто добавить лямбду и параметр в очередь !
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

    void Register(const unique_ptr<CommandsHandler> &_cmds)
    {
        _cmds->subscribe(shared_from_this());
    }

    void Do(/*const*/ CommandBlocksType &cmds_blocks, [[maybe_unused]] time_t t) override
    {
        {
            std::lock_guard<std::mutex> lk(cv_m);

            q_functions.emplace( [](CommandsType _block)
                {
                    // здесь и далее в лямбде - код, который будет выполнятся не сейчас, а когда-то позже в отдельном потоке
                    if (_block.empty())
                        return;   // если кто-то до этого момента уже исполнил блок, то делать нечего - выходим

                    stringstream s;     // Я так понимаю, что если бы у нас будет больше одного ConsoleObserver то начнет конка за этот s. Верно???
                    //console_m.lock();
                    s << "bulk: ";

                    size_t cmds_size = _block.size(); // _block - это локальая копия? можно не копировать???

                    s << "(size = " << cmds_size << ") : ";
                    //console_m.unlock();

                    for (size_t i = 0; i < cmds_size; i++)
                    {
                        unsigned long long fa_res = fa(stoi(_block[i]));
                        s << fa_res << (  (i<(cmds_size-1)) ? ", " : "\n");
                    }

                    console_m.lock();
                    std::cout << s.str();
                    console_m.unlock();

                } // конец лямбды
                , cmds_blocks);
        }
        cv.notify_one();  // vs. cv.notify_all() ?????????
        //cv.notify_all();


          // далее код от обычной бульки
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
    //std::mutex file_m; // вспомогательный мьютекс для LocalFileObserver
public:

    void Register(const unique_ptr<CommandsHandler> &_cmds)
    {
        _cmds->subscribe(shared_from_this());
    }

    void Do(/*const*/ CommandBlocksType &cmds_blocks, time_t t) override
    {    
        {
            std::lock_guard<std::mutex> lk(cv_m); // все, что дальше этой строчки будет выполнено только в одном потоке???
//            if (cmds_block.empty())
//                return;   // если кто-то до этого момента уже исполнил блок, то делать нечего - выходим
            //q_functions.emplace(std::async( std::launch::deferred, [t, this](CommandsType _block) // Если бы было [cmds_block, t] то cmds - это же копия ??? т.е. можно ее уже не блокировать и использовать в том виде, в котором она пришла?


            q_functions.emplace( [t, this](CommandsType _block)
                {
                    // здесь и далее в лямбде - код, который будет выполнятся не сейчас, а когда-то позже в отдельном потоке
                    if (_block.empty())
                        return;   // если кто-то до этого момента уже исполнил блок, то делать нечего - выходим

                    static std::atomic_int nFile = 0; // Сквозная нумерация // Нужно делать атомиком???

                    stringstream s; // Этот s у каждой лямбды свой или может быть подстава с общим доступом (data racing)?
                    s << "bulk" << t << "-" << std::this_thread::get_id() << "-" << nFile++ << ".log";
                    ofstream f( s.str() );

                    size_t cmds_block_size = _block.size();

                    for (size_t i = 0; i < cmds_block_size; i++)
                    {
                        //file_m.lock();
                        unsigned long long fi_param = stoi(_block[i]); // сейчас _block - локальная копия
                        //file_m.unlock();
                        // далее само долгое вычисления пусть будет незалоченым и выполняется параллельно
                        unsigned long long fi_res = fi(fi_param); // если cmds_block - локальная копия, то можно не лочить, верно?

                        f << fi_res << std::endl; // файловые потоки не лочим, т.к. они все уникальные для каждого потока. Верно?
                    }

                    f.close();

                },  // конец лямбды
                cmds_blocks);

//            cmds.clear();  // удаляем блок команд, чтобы больше никому не достался
        }
        cv.notify_one(); // vs. cv.notify_all() ????????
        //cv.notify_all();


//        stringstream s; // далее - остатки старой обычной бульки
//        s << "bulk" << t << "-" << std::this_thread::get_id() << ".log";
//        ofstream f( s.str() );

//        size_t cmds_size = cmds.size();
//        for (size_t i = 0; i < cmds_size; i++)
//            f << fi(stoi(cmds[i])) << std::endl;

//        f.close();
    }
};
//-----------------------------------------------




