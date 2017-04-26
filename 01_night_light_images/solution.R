library(raster)

# Load the image and crop it to hanover
r = raster("images/BlackMarble_2012_C1_geo.tif")
r.hanover = crop(r,extent(c(9,10,52,53)))

r2 = raster("images/BlackMarble_2016_C1_geo.tif")
r2.hanover = crop(r2,extent(c(9,10,52,53)))

pdf("plots.pdf") # remove this in an interactive session

plot(r.hanover, main="Hanover at night in 2012")
points(9.71730046412,52.375993496,pch=23,bg="red",col="red",cex=2) 

plot(r2.hanover, main="Hanover at night in 2016")
points(9.71730046412,52.375993496,pch=23,bg="red",col="red",cex=2) 

plot(r.hanover - r2.hanover, main="Difference of Hanover at night")
points(9.71730046412,52.375993496,pch=23,bg="red",col="red",cex=2) 

dev.off();
