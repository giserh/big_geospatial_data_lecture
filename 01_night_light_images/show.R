library("raster")
r = raster("images/BlackMarble_2012_C1_geo.tif")
r2 = raster("images/BlackMarble_2016_C1_geo.tif")
e <- extent(9,10, 52, 53) # Hanover GPS range
re = crop(r,e)
re2 = crop(r2,e);
par(mfrow=c(1,2))
plot(re)
plot(re2)

