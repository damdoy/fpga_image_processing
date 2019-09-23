#!/bin/bash

gnuplot --persist -e "set palette gray; set yrange [] reverse; set cbrange [0:255]; set palette defined (0 \"black\", 255 \"white\"); plot \"output.dat\" matrix w image noti"
