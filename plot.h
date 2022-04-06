#pragma once

#define EPSILON (1e-16)

#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <bits/stdc++.h>

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

struct AxisData
{
    std::string name;
    double lowerBound, upperBound;
    int numOfValues;
};

// struct Axis {
//     sf::Vertex line[2];
//     AxisData data;
// };

struct ColorValue
{
    std::uint8_t r, g, b;
};

static ColorValue get_viridis_color(const double t)
{
    double r = -1075.3 * pow(t, 4) + 2798.3 * pow(t, 3) - 1797.7 * t * t + 264.69 * t + 65.689;
    double g = -115.36 * t * t + 347.95 * t + 1.4182;
    double b = 3580.8 * pow(t, 5) - 8436.8 * pow(t, 4) + 6989.3 * pow(t, 3) - 2765.6 * t * t + 585.16 * t + 83.295;
    return {static_cast<std::uint8_t>(r), static_cast<std::uint8_t>(g), static_cast<std::uint8_t>(b)};
}

using ZMap = std::map<std::pair<double, double>, double>;
using HeatMap = std::vector<sf::RectangleShape>;

class Plot
{
public:
    Plot() = delete;
    static void plot(const std::string &&filename)
    {
        parse_csv(std::move(filename));
        fix_possible_error();
        assign_colors();
        position_and_scale();
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
                x = std::atof(value.c_str());
                first_x_value = x;

                std::getline(csv_file, value, ',');
                y = std::atof(value.c_str());

                std::getline(csv_file, value, '\n');
                z = std::atof(value.c_str());

                m_zMap[std::pair(x, y)] = z;

                // Now all other rows are calculated
                while (!csv_file.eof())
                {
                    std::getline(csv_file, value, ',');
                    x = std::atof(value.c_str());
                    if(!number_of_unique_y_values_found){
                        if(fabs(x - first_x_value) < EPSILON){
                            number_of_unique_y_values++;

                        }
                        else if (fabs(x - first_x_value) > EPSILON){
                            number_of_unique_y_values_found = true;
                        }
                    }

                    std::getline(csv_file, value, ',');
                    y = std::atof(value.c_str());

                    std::getline(csv_file, value, '\n');
                    z = std::atof(value.c_str());

                    m_zMap[std::pair(x, y)] = z;
                }
            }

            // Setting up boundaries
            m_xAxisData.lowerBound = (*m_zMap.begin()).first.first;
            m_yAxisData.lowerBound = (*m_zMap.begin()).first.second;
            m_zAxisData.lowerBound = (std::min_element(m_zMap.begin(), m_zMap.end(), [](const auto &l, const auto &r)
                                                       { return l.second < r.second; }))
                                         ->second;

            m_xAxisData.upperBound = (*m_zMap.rbegin()).first.first;
            m_yAxisData.upperBound = (*m_zMap.rbegin()).first.second;
            m_zAxisData.upperBound = (std::max_element(m_zMap.begin(), m_zMap.end(), [](const auto &l, const auto &r)
                                                       { return l.second < r.second; }))
                                         ->second;

            m_xAxisData.numOfValues = m_zMap.size() / number_of_unique_y_values;
            m_yAxisData.numOfValues = number_of_unique_y_values;
            m_zAxisData.numOfValues = m_zMap.size();

            csv_file.close();
        }
    }

    /* ++++++++++++++++++++++++++++++++++++++++++ */
    /*           fix possible error               */
    /* ++++++++++++++++++++++++++++++++++++++++++ */
    static void fix_possible_error(){
        if (m_zAxisData.numOfValues > (m_xAxisData.numOfValues * m_yAxisData.numOfValues))
        {
            int i{0};
            for (auto it = m_zMap.begin(); it != m_zMap.end(); ++it)
            {
                if (i++ == (m_zAxisData.numOfValues +m_yAxisData.numOfValues)/ 2)
                {
                    std::cout << "Here!";
                    m_zMap.erase(it);
                    return;
                }
            }
        }
    }

    /* ++++++++++++++++++++++++++++++++++++++++++ */
    /*              assign colors                 */
    /* ++++++++++++++++++++++++++++++++++++++++++ */
    static void assign_colors()
    {
        m_heatMap.resize(m_zMap.size());
        auto affine = [&](double value){
            static double k = 1.0f / (m_zAxisData.upperBound - m_zAxisData.lowerBound);
            static double c = -k * m_zAxisData.lowerBound;
            return static_cast<double>(k * value + c);
        };

        long long index = 0;
        for (auto &[arg, z] : m_zMap)
        {
            ColorValue colorValue{get_viridis_color(affine(z))};
            m_heatMap.at(index).setFillColor({colorValue.r, colorValue.g, colorValue.b});
            index++;
        }
    }

    /* ++++++++++++++++++++++++++++++++++++++++++ */
    /*       position and scale rectangles        */
    /* ++++++++++++++++++++++++++++++++++++++++++ */
    static void position_and_scale()
    {
        sf::Vector2f tileSize{
            800.0f / static_cast<float>(m_xAxisData.numOfValues),
            800.0f / static_cast<float>(m_yAxisData.numOfValues)};
        sf::Vector2f baseTilePosition{
            100.0f + tileSize.x / 2.0f,
            100.0f + tileSize.y / 2.0f};
        int xOffset{0}, yOffset{0};

        for (auto &tile : m_heatMap)
        {
            tile.setSize(tileSize);
            tile.setOrigin({tile.getSize().x / 2.0f, tile.getSize().y / 2.0f});
            tile.setPosition({baseTilePosition.x + xOffset * tile.getSize().x,
                              baseTilePosition.y + yOffset * tile.getSize().y});
            yOffset++;
            if (yOffset % (m_yAxisData.numOfValues) == 0)
            {
                yOffset %= m_yAxisData.numOfValues;
                xOffset++;
            }
        }
    }

    /* ++++++++++++++++++++++++++++++++++++++++++ */
    /*               display plot                 */
    /* ++++++++++++++++++++++++++++++++++++++++++ */
    static void display(const std::string &&filename)
    {
        // SFML stuff

        sf::RenderWindow window{sf::VideoMode{1000, 1000}, filename + " plot", sf::Style::Default};
        window.setActive();

        while (window.isOpen())
        {
            window.clear(sf::Color::White);

            sf::Event e;
            while (window.pollEvent(e))
            {
                if (e.type == sf::Event::Closed)
                {
                    window.close();
                }
            }
            // Render axis as well
            for (auto &tile : m_heatMap)
            {
                window.draw(tile);
            }
            window.display();
        }

        get_viridis_color(0.5);
    }
};