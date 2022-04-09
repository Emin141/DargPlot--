#include "plot.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <bits/stdc++.h>

#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <X11/Xlib.h>

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

void Plot::plot(const std::string &&filename)
{
    XInitThreads();
    parse_csv(std::move(filename));
    fix_possible_error();
    assign_colors();
    position_and_scale();
    draw_axis_data();
    display(std::move(filename));
}

/* ++++++++++++++++++++++++++++++++++++++++++ */
/*                 csv parsing                */
/* ++++++++++++++++++++++++++++++++++++++++++ */
void Plot::parse_csv(const std::string &&filename)
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
        m_xAxis.data.name = value;
        std::getline(csv_file, value, ',');
        m_yAxis.data.name = value,
        std::getline(csv_file, value, '\n');
        m_zAxis.data.name = value;

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
                if (!number_of_unique_y_values_found)
                {
                    if (fabs(x - first_x_value) < EPSILON)
                    {
                        number_of_unique_y_values++;
                    }
                    else if (fabs(x - first_x_value) > EPSILON)
                    {
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
        m_xAxis.data.lowerBound = (*m_zMap.begin()).first.first;
        m_yAxis.data.lowerBound = (*m_zMap.begin()).first.second;
        m_zAxis.data.lowerBound = (std::min_element(m_zMap.begin(), m_zMap.end(), [](const auto &l, const auto &r)
                                                    { return l.second < r.second; }))
                                      ->second;

        m_xAxis.data.upperBound = (*m_zMap.rbegin()).first.first;
        m_yAxis.data.upperBound = (*m_zMap.rbegin()).first.second;
        m_zAxis.data.upperBound = (std::max_element(m_zMap.begin(), m_zMap.end(), [](const auto &l, const auto &r)
                                                    { return l.second < r.second; }))
                                      ->second;

        m_xAxis.data.numOfValues = m_zMap.size() / number_of_unique_y_values;
        m_yAxis.data.numOfValues = number_of_unique_y_values;
        m_zAxis.data.numOfValues = m_zMap.size();

        csv_file.close();
    }
}

/* ++++++++++++++++++++++++++++++++++++++++++ */
/*           fix possible error               */
/* ++++++++++++++++++++++++++++++++++++++++++ */
void Plot::fix_possible_error() noexcept
{
    if (m_zAxis.data.numOfValues > (m_xAxis.data.numOfValues * m_yAxis.data.numOfValues))
    {
        int i{0};
        for (auto it = m_zMap.begin(); it != m_zMap.end(); ++it)
        {
            if (i++ == (m_zAxis.data.numOfValues + m_yAxis.data.numOfValues) / 2)
            {
                m_zMap.erase(it);
                return;
            }
        }
    }
}

/* ++++++++++++++++++++++++++++++++++++++++++ */
/*              assign colors                 */
/* ++++++++++++++++++++++++++++++++++++++++++ */
void Plot::assign_colors() noexcept
{
    m_heatMap.resize(m_zMap.size());
    auto affine = [&](const double value)
    {
        static double k = 1.0f / (m_zAxis.data.upperBound - m_zAxis.data.lowerBound);
        static double c = -k * m_zAxis.data.lowerBound;
        return static_cast<double>(k * value + c);
    };

    int index{0};
    for (auto &[arg, z] : m_zMap)
    {
        auto [r, g, b] = get_viridis_color(affine(z));
        m_heatMap.at(index).setFillColor(sf::Color{r, g, b});
        index++;
    }
}

/* ++++++++++++++++++++++++++++++++++++++++++ */
/*       position and scale rectangles        */
/* ++++++++++++++++++++++++++++++++++++++++++ */
void Plot::position_and_scale() noexcept
{
    sf::Vector2f tileSize{
        800.0f / static_cast<float>(m_xAxis.data.numOfValues),
        800.0f / static_cast<float>(m_yAxis.data.numOfValues)};
    sf::Vector2f baseTilePosition{
        100.0f + tileSize.x / 2.0f,
        100.0f + tileSize.y / 2.0f};
    int xOffset{0}, yOffset{0};

    for (auto &tile : m_heatMap)
    {
        tile.setSize(tileSize);
        tile.setOrigin({tileSize.x / 2.0f, tileSize.y / 2.0f});
        tile.setPosition({baseTilePosition.x + xOffset * tileSize.x,
                          baseTilePosition.y + yOffset * tileSize.y});
        yOffset++;
        if (yOffset % (m_yAxis.data.numOfValues) == 0)
        {
            yOffset %= m_yAxis.data.numOfValues;
            xOffset++;
        }
    }
}

/* ++++++++++++++++++++++++++++++++++++++++++ */
/*              draw axis data                */
/* ++++++++++++++++++++++++++++++++++++++++++ */
void Plot::draw_axis_data()
{
    if(!m_font.loadFromFile("CascadiaMono-ExtraLight.otf")){
        throw std::runtime_error("Fatal error: could not load font \"CascadiaMono-ExtraLight.otf\"");
    }
    auto shared_setup = [&](Axis &axis)
    {
        // Title
        axis.text.push_back({axis.data.name, m_font, 32});
        // TODO: custom number of divisions
        // Five divisions
        double step = (axis.data.upperBound - axis.data.lowerBound) / 5.0f;
        for (int i = 0; i <= 5; i++)
        { // max value may not be correct?
            std::stringstream value{};
            value << std::setprecision(2) << (axis.data.lowerBound + step * i);
            axis.text.push_back({value.str(), m_font, 32});
        }
        for (auto &text : axis.text)
        {
            text.setFillColor(sf::Color::Black);
            sf::FloatRect bounds = text.getGlobalBounds();
            text.setOrigin(
                bounds.left + bounds.width / 2.0f,
                bounds.top + bounds.height / 2.0f);
        }
    };

    shared_setup(m_xAxis);
    shared_setup(m_yAxis);
    shared_setup(m_zAxis);

    // Position setup is individual
    m_xAxis.text.at(0).setPosition(50.0f, 50.0f);
    double offset{0.0f};
    for(auto it = ++m_xAxis.text.begin(); it != m_xAxis.text.end(); ++it)
    {
        it->setPosition(50.0f, 100.0f + offset);
        offset += 160.0f;
    }


    m_yAxis.text.at(0).setPosition(950.0f, 950.0f);
    offset = 0.0f;
    for(auto it = ++m_yAxis.text.begin(); it != m_yAxis.text.end(); ++it)
    {
        it->setPosition(100.0f + offset, 950.0f);
        offset += 160.0f;
    }
}

/* ++++++++++++++++++++++++++++++++++++++++++ */
/*                draw legend                 */
/* ++++++++++++++++++++++++++++++++++++++++++ */
void Plot::draw_legend() noexcept{
    // Legends window
    sf::RenderWindow legend{sf::VideoMode{300, 1000}, m_zAxis.data.name + " legend", sf::Style::Default};
    legend.setPosition({1000, 0});
    legend.clear(sf::Color::White);
    for(double i=0; i<=100; i+=1.0f){
        sf::RectangleShape gradientLine{{50.0f, 8.0f}};
        gradientLine.setPosition(50.0f, 100.0f + 8.0f*static_cast<float>(i));
        auto [r, g, b] = get_viridis_color(i/100.0f);
        gradientLine.setFillColor(sf::Color{r, g, b});
        legend.draw(gradientLine);
    }
    double offset{0.0f};
    m_zAxis.text.at(0).setPosition(200.0f, 50.0f);
    legend.draw(m_zAxis.text.at(0));
    for(auto it=++m_zAxis.text.begin(); it!=m_zAxis.text.end(); ++it){
        it->setPosition(200.0f, 100.0f + offset);
        legend.draw(*it);
        offset += 160.0f;
    }
    legend.display();
    while(legend.isOpen()){
        sf::Event e;
        while(legend.pollEvent(e)){
            if(e.type == sf::Event::Closed){
                legend.close();
            }
        }        
    }
}

/* ++++++++++++++++++++++++++++++++++++++++++ */
/*               display plot                 */
/* ++++++++++++++++++++++++++++++++++++++++++ */
void Plot::display(const std::string &&filename) noexcept
{
    // SFML stuff
    sf::Thread legendThread{draw_legend};
    legendThread.launch();

    sf::RenderWindow window{sf::VideoMode{1000, 1000}, filename + " plot", sf::Style::Default};
    window.setPosition({0, 0});
    window.setActive();

    // Plot title
    sf::Text title{
        m_zAxis.data.name + "(" + m_xAxis.data.name + ", " + m_yAxis.data.name + ")",
        m_font,
        48};
    title.setFillColor(sf::Color::Black);
    sf::FloatRect bounds{title.getGlobalBounds()};
    title.setOrigin(
        bounds.left + bounds.width / 2.0f,
        bounds.top + bounds.height / 2.0f);
    title.setPosition(500.0f, 50.0f);


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
        window.draw(title);
        for (auto &text : m_xAxis.text)
        {
            window.draw(text);
        }
        for (auto &text : m_yAxis.text)
        {
            window.draw(text);
        }
        for (auto &tile : m_heatMap)
        {
            window.draw(tile);
        }
        window.display();
    }

    legendThread.terminate();
}
