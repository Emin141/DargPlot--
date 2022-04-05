#include <iostream>
#include "plot.h"

int main(int argc, char **argv)
{
    try
    {
        if (argc < 2)
        {
            throw std::runtime_error("Fatal error: too few parameters. Please specify a file to be processed.");
        }
        Plot::plot(argv[1]);
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << std::endl;
    }
    return 0;
}