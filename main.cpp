#include <iostream>
#include <SFML/System.hpp>

int main(int argc, char **argv)
{
    try
    {
        std::cout << argv[0] << std::endl;
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << std::endl;
    }
    return 0;
}