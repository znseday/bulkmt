#ifndef METRICS_H
#define METRICS_H

#include <atomic>
#include <iostream>

struct MetricsLocalThread
{
    int nBlocks = 0;
    int nCommands = 0;

    friend std::ostream & operator<<(std::ostream &s, const MetricsLocalThread &ob)
    {
        s << ob.nBlocks << " block(s), " << ob.nCommands << " command(s)";
        return s;
    }
};

struct MetricsMainThread : MetricsLocalThread
{
    /*std::atomic_int*/ //int nBlocks = 0;
    /*std::atomic_int*/ //int nCommands = 0;
    /*std::atomic_int*/ int nLines = 0;

    friend std::ostream & operator<<(std::ostream &s, const MetricsMainThread &ob)
    {
        s << ob.nLines << " lines, ";
        s << static_cast<const MetricsLocalThread&>(ob);
        return s;
    }
};

#endif // METRICS_H
