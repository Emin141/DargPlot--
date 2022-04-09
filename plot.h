#pragma once

#define EPSILON (1e-16)

#include <map>
#include <vector>
#include <string>

#include <SFML/Graphics.hpp>


struct Axis
{
    struct AxisData
    {
        std::string name;
        double lowerBound, upperBound;
        int numOfValues;
    } data;
    std::vector<sf::Text> text;
};

using ZMap = std::map<std::pair<double, double>, double>;
using HeatMap = std::vector<sf::RectangleShape>;

class Plot
{
public:
// Singleton
    Plot() = delete;
    Plot(Plot const &) = delete;
    void operator=(Plot const &) = delete;

    static void plot(const std::string &&filename);

private:
    /* ++++++++++++++++++++++++++++++++++++++++++ */
    /*              Member variables              */
    /* ++++++++++++++++++++++++++++++++++++++++++ */
    inline static Axis m_xAxis, m_yAxis, m_zAxis;
    inline static ZMap m_zMap;
    inline static HeatMap m_heatMap;
    inline static sf::Font m_font;

    static void parse_csv(const std::string &&filename);
    static void fix_possible_error() noexcept;
    static void assign_colors() noexcept;
    static void position_and_scale() noexcept;
    static void draw_axis_data();
    static void draw_legend() noexcept;
    static void display(const std::string &&filename) noexcept;
};