// Some source adapted from http://www.dash-project.org/tutorial/tutorial.html#astro_load_image

#include <iostream>
#include <chrono>
#include<stdexcept>
#include<algorithm>

using std::cout;
using std::endl;
using std::flush;
using std::runtime_error;
using std::chrono::system_clock;


extern "C" {
    #include <tiffio.h>
    #include <png.h>
}

/*
libPNG is a very stable library for writing imagery to disk. 
This function encapsulates this C-style library. It expects a
filename, width and height, and an array of pointers each pointing
to the beginning of each line. Image data is assumed to be RGB 
with 8 bit per channel.
*/
void libpng_write(const char *fname, size_t exc_width, size_t exc_height,
		    png_bytep *row_pointers);

// structure helping us to use RGB
struct RGB
{
  uint8 r;
  uint8 g;
  uint8 b;
  uint32_t brightness() const { return (uint32_t) r + (uint32_t) g + (uint32_t) b; }
  RGB():r(0),g(0),b(0){};
  RGB(uint8 _r,uint8 _g,uint8 _b): r(_r),g(_g),b(_b){};
};



int main( int argc, char* argv[] ) {
    if (argc != 2)
      throw(runtime_error("need a tif file as argument"));

    uint32_t rowsperstrip= 0;
    TIFF* tif= NULL;
    std::chrono::time_point<std::chrono::system_clock> start, end;

        
     tif = TIFFOpen( argv[1], "r" );

     if ( tif == NULL ) 
            throw(std::runtime_error("not a TIF"));
        

        uint32_t bitsps= 0;
        uint32_t samplespp= 0;

        uint32_t width  = 0;
        uint32_t height = 0;

        TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width );
        TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height );
        TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitsps );
        TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplespp );
        TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip );

        cout << "input file " << argv[1] << endl << "    image size: " << width << " x "
             << height << " pixels with " << samplespp << " channels x " << bitsps
             << " bits per pixel, " << rowsperstrip << " rows per strip" << endl;

    // Now read the image
        uint32_t numstrips= TIFFNumberOfStrips( tif );
        tdata_t buf= _TIFFmalloc( TIFFStripSize( tif ) );

        std::vector<RGB> matrix(height*width);
        auto iter= matrix.begin();

        uint32_t line= 0;
        start= system_clock::now();
        for ( uint32_t strip = 0; strip < numstrips; strip++, line += rowsperstrip ) {

            TIFFReadEncodedStrip( tif, strip, buf, (tsize_t) -1 );
            RGB* rgb= (RGB*) buf;

            /*std::for_each( rgb, rgb + width * rowsperstrip, [](RGB& rgb) {
                std::swap<uint8_t>( rgb.r, rgb.b );
            } );*/


            for ( uint32_t l= 0; ( l < rowsperstrip ) && (line+l < height); l++ ) {

                iter = std::copy( rgb, rgb+width, iter );
                rgb += width;
            }

            if ( 0 == ( strip % 100 ) ) {
                cout << "    strip " << strip << "/" << numstrips << "\r" << flush;
            }
        }

        end= system_clock::now();
         cout << "read image in "<< std::chrono::duration_cast<std::chrono::seconds> (end-start).count() << " seconds" << endl;
        /*
	    Now, we can process the image
	*/
       start = system_clock::now();



       end = system_clock::now();
	 cout << "processed image in "<< std::chrono::duration_cast<std::chrono::seconds> (end-start).count() << " seconds" << endl;
	 
	/*Now let us look at some small excerpts writing PNG
	to disk*/
	size_t exc_height = 500, exc_width=500;
    std::cout << "Writing some excerpts" << endl;
    start = std::chrono::system_clock::now();

    #pragma omp parallel for
    for (size_t k=0; k < 10; k++)
    {
        size_t exc_x = rand() % (width-exc_width-1);
	size_t exc_y = rand() % (height-exc_height-1);
	if (exc_height + exc_y >= height ||
	    exc_width + exc_x >= width)
	    throw(std::runtime_error("Excerpt not inside"));
      png_bytep* row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * exc_height);
        for (size_t y=0; y<exc_height; y++)
                row_pointers[y] = &matrix[(y+exc_y)*width+exc_x].r;
      char fname[1024];
      snprintf(fname,1024,"out-%04d.png",k);
    libpng_write(fname,exc_width,exc_height,row_pointers);
    free(row_pointers);
    }
      end= std::chrono::system_clock::now();
      cout << "wrote images in "<< std::chrono::duration_cast<std::chrono::seconds> (end-start).count() << " seconds" << endl;
    
    return 0;
}

void libpng_write(const char *fname, size_t exc_width, size_t exc_height,
		    png_bytep *row_pointers)
{
	/*actual write (C-Style libpng)*/


        FILE *fp = fopen(fname, "wb");
        if (!fp)
	  throw(std::runtime_error("Problem writing PNG"));

       auto  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

        if (!png_ptr)
		  throw(std::runtime_error("Problem writing PNG"));

        auto info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
	  throw(std::runtime_error("Problem writing PNG"));

        if (setjmp(png_jmpbuf(png_ptr)))
	  throw(std::runtime_error("Problem writing PNG"));
	  
        png_init_io(png_ptr, fp);


        /* write header */
        if (setjmp(png_jmpbuf(png_ptr)))
		  throw(std::runtime_error("Problem writing PNG"));


        png_set_IHDR(png_ptr, info_ptr, exc_width, exc_height,
                     8, 2, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

        png_write_info(png_ptr, info_ptr);


        /* write bytes */
        if (setjmp(png_jmpbuf(png_ptr)))
	  throw(std::runtime_error("Problem writing PNG"));
		  
               

        png_write_image(png_ptr, row_pointers);


        /* end write */
        if (setjmp(png_jmpbuf(png_ptr)))
	  throw(std::runtime_error("Problem writing PNG"));
	  

        png_write_end(png_ptr, NULL);

        /* cleanup heap allocation */
        
        fclose(fp);
}
    

