# Docker image for the lecture Big Geospatial Data

This repository contains the sources needed to build a docker image for
use with the lecture Big Geospatial Data held by M. Werner at University of
Hanover.

It contains several components used during the tutorial of the lecture and is
in general a good starting point for spatial data analysis.

The current version consists of
- Debian Base
- Git and C/C++ development environment
- R 3.3 from CRAN with all recommended packages

If you want to run it, just install docker and do

         docker run -it mwernerds/bgd:latest bash

This will bring download it, if needed. If you want
to share a directory or to use the X windows system (for interactive
plotting for example), just use the xrun script

	 xrun mwernerds/bgd:latest bash

This script (read it!) opens X windows for the new guest, so it can use your monitor and mounts
the subdirectory data to ~/data on docker as a shared directory. Just copy files there and they
are visible both on the host and inside the container.

The script xrun is, however, Linux-specific and won't work on Mac or Windows.

