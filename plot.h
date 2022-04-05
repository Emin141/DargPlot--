#pragma once

#define EPSILON (10e-10)

#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

struct AxisData
{
    std::string name;
    double lowerBound, upperBound, numOfValues;
};

struct Axis {
    sf::Vertex line[2];
    AxisData data;
};

struct ColorValue
{
    double r, g, b;
};

static ColorValue get_viridis_color(const double t){
    double r = -1075.3 * pow(t, 4) + 2798.3 * pow(t, 3) - 1797.7 * t * t + 264.69 * t + 65.689;
	double g = -115.36 * t * t + 347.95 * t + 1.4182;
	double b = 3580.8 * pow(t, 5) - 8436.8 * pow(t, 4) + 6989.3 * pow(t, 3) - 2765.6 * t * t + 585.16 * t + 83.295;
    return {r, g, b};
}

using ZMap = std::map<std::pair<double, double>, double>;
using HeatMap = std::pair<double, ColorValue>;

class Plot
{
public:
    Plot() = delete;
    static void plot(const std::string &&filename)
    {
        parse_csv(std::move(filename));
        display(std::move(filename));
    }

    Plot(Plot const &) = delete;
    void operator=(Plot const &) = delete;

private:
    /* ++++++++++++++++++++++++++++++++++++++++++ */
    /*              Member variables              */
    /* ++++++++++++++++++++++++++++++++++++++++++ */
    inline static AxisData m_xAxisData, m_yAxisData, m_zAxisData;
    inline static ZMap m_zMap;
    inline static HeatMap m_heatMap;

    /* ++++++++++++++++++++++++++++++++++++++++++ */
    /*                 csv parsing                */
    /* ++++++++++++++++++++++++++++++++++++++++++ */
    static void parse_csv(const std::string &&filename)
    {
        std::ifstream csv_file{filename, std::ios::in};
        if (!csv_file.is_open())
        {
            throw std::runtime_error("Could not read CSV file " + filename);
        }
        else
        {
            std::string value{};

            // Getting the label names
            std::getline(csv_file, value, ',');
            m_xAxisData.name = value;
            std::getline(csv_file, value, ',');
            m_yAxisData.name = value,
            std::getline(csv_file, value, '\n');
            m_zAxisData.name = value;

            // IMPORTANT
            // Need to get the first X value in order to check for the number of y values
            double first_x_value{};
            int number_of_unique_y_values{1};
            bool number_of_unique_y_values_found{false};

            {
                double x, y, z;

                std::getline(csv_file, value, ',');
                x = atof(value.c_str());
                first_x_value = x;

                std::getline(csv_file, value, ',');
                y = atof(value.c_str());

                std::getline(csv_file, value, '\n');
                z = atof(value.c_str());

                m_zMap[std::pair(x, y)] = z;


                // Now all other rows are calculated
                while (!csv_file.eof())
                {
                    std::getline(csv_file, value, ',');
                    x = atof(value.c_str());
                    if (fabs(x - first_x_value) < EPSILON and !number_of_unique_y_values_found)
                    {
                        number_of_unique_y_values++;
                    }
                    else if (fabs(x-first_x_value)> EPSILON and !number_of_unique_y_values_found)
                    {
                        number_of_unique_y_values_found = true;
                    }

                    std::getline(csv_file, value, ',');
                    y = atof(value.c_str());

                    std::getline(csv_file, value, '\n');
                    z = atof(value.c_str());

                    m_zMap[std::pair(x, y)] = z;
                }
            }

            // Setting up boundaries
            m_xAxisData.lowerBound = (*m_zMap.begin()).first.first;
            m_yAxisData.lowerBound = (*m_zMap.begin()).first.second;
            m_zAxisData.lowerBound = (*m_zMap.begin()).second;

            m_xAxisData.upperBound = (*m_zMap.rbegin()).first.first;
            m_yAxisData.upperBound = (*m_zMap.rbegin()).first.second;
            m_zAxisData.upperBound = (*m_zMap.rbegin()).second;

            m_xAxisData.numOfValues = m_zMap.size() / number_of_unique_y_values;
            m_yAxisData.numOfValues = number_of_unique_y_values;
            m_zAxisData.numOfValues = m_zMap.size();

            csv_file.close();
        }
    }

    /* ++++++++++++++++++++++++++++++++++++++++++ */
    /*               display plot                 */
    /* ++++++++++++++++++++++++++++++++++++++++++ */
    static void display(const std::string &&filename)
    {
        // SFML stuff

        sf::RenderWindow window{sf::VideoMode{800, 800}, filename + " plot", sf::Style::Default};
        window.setActive();

        while(window.isOpen())
        {
            window.clear(sf::Color::White);
            sf::Event e;
            while(window.pollEvent(e)){
                if(e.type == sf::Event::Closed){
                    window.close();
                }
            }

            window.display();
        }

        get_viridis_color(0.5);
    }
};