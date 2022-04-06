#!/bin/bash

g++ generator.cpp -o generator && ./generator test.csv && ./dargplot test.csv ;