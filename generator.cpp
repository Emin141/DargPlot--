#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <random>

int main(int argc, char **argv){
    try
    {
        if (argc < 2)
        {
            throw std::runtime_error("Fatal error: too few parameters. Please specify a file to be written to.");
        }

        std::ofstream file{argv[1], std::ios::out};
        if(!file.is_open()){
            throw std::runtime_error("Fatal error: failed to create file with name " + std::string{argv[1]} + ".");
        }

        file << "x,y,z\n";
        auto z = [](double x, double y){
            double r = std::sqrt(x * x + y * y);
            return std::sin(0.7f*r)/r;
            };

        for(double i=-10.0f; i<=10.0f; i+=0.02){
            for(double j=-10.0f; j<=10.0f; j+=0.02){
                file << i << "," << j << "," << z(i,j) << "\n";
            }
        }
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << std::endl;
    }
    return 0;
}