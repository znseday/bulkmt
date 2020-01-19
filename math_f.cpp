#include "math_f.h"

#include <stdexcept>

unsigned long long MyFa(unsigned long long n)
{
    unsigned long long res = 1;
    for (unsigned long long i = 2; i <= n; i++)
        res *= i;

    return res;
}

unsigned long long MyFi(unsigned long long n)
{
    if (n <= 0) return 0;
    if (n == 1 || n == 2) return 1;
    if (n > 55)
        throw std::invalid_argument("fi: n > 55");
    return fi(n-1) + fi(n-2);
}
