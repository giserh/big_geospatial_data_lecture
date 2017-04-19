# Prepare C++ environment

In addition to a C++ compiler and make, you will need libpng and libtiff.
On Debian or Ubuntu, these are provided as packages

- libpng-dev
- libtiff-dev

Information on the dataset is available at
<https://earthobservatory.nasa.gov/Features/NightLights/page3.php>



# Prepare R for this type of data

Install GDAL for your system, for example

	sudo apt-get install libgdal-dev libproj-dev

Install rgdal, z.B.

	sudo R
	> install.packages("rgdal")
	> install.packages("raster");


