#include <iostream>
#include <vector>
#include "../../lib/cpptoml/cpptoml.h"
#include "fitsio.h"  /* required by every program that uses CFITSIO  */


int saveFITS(string filename, auto config, vector<int> fitsArray, times timesStruct)
{   /* Create a FITS primary array containing a 2-D image */
    fitsfile *fptr;       /* pointer to the FITS file; defined in fitsio.h */


    string filedir = config->get_qualified_as<long>("fits.bufferSize").value_or(0);
    string filepath = filedir + filename;             /* name for new FITS file */
    int bitpix  =  16;         /* 16-bit short signed integer pixel values */
    long naxis  =  3;        /* 2-dimensional image                      */
    long bufferSize = config->get_qualified_as<long>("fits.bufferSize").value_or(0);
    long naxes[3] = {chunkArray[0].width, chunkArray[0].height, bufferSize};   /* image is 300 pixels wide by 200 rows */
    int status = 0;         /* initialize status before calling fitsio routines */

    if (fits_create_file(&fptr, filepath, &status)) /* create new FITS file */
        return( status );

    /* Write the required keywords for the primary array image */
    if ( fits_create_img(fptr,  bitpix, naxis, naxes, &status) )
         return( status );

    long fpixel = 1;                               /* first pixel to write      */
    long nelements = naxes[0] * naxes[1] * naxes[2];          /* number of pixels to write */

    /* Write the array of long integers (after converting them to short) */
    if ( fits_write_img(fptr, TINT, fpixel, nelements, array[0], &status) )
        return( status );

    int width = config->get_qualified_as<int>("camera.width").value_or(0);
    int height = config->get_qualified_as<int>("camera.height").value_or(0);
    int offset_x = config->get_qualified_as<int>("camera.offset_x").value_or(0);
    int offset_y = config->get_qualified_as<int>("camera.offset_y").value_or(0);
    int exposure_time = config->get_qualified_as<int>("camera.exposure_time").value_or(0);

    // Write TotalExposureTime //
    if ( fits_write_key(fptr, TSTRING, "STARTTIME", &timesStruct.timestamp,
         "Timestamp of beginning of exposure", &status) )
         return( status );

    // Write IndividualExposureTime //
    if ( fits_write_key(fptr, TDOUBLE, "FRAMEEXPOSURE", &exposureTime,
         "Individual Exposure Time (us)", &status) )
         return( status );

    // Write TotalExposureTime //
    if ( fits_write_key(fptr, TINT, "TOTALEXPOSURE", &timesStruct.totalexposure,
         "Total Exposure Time (s)", &status) )
         return( status );

    // Write Height //
    if ( fits_write_key(fptr, TLONG, "HEIGHT", &height,
         "Image Height (px)", &status) )
         return( status );

    // Write Width //
    if ( fits_write_key(fptr, TLONG, "WIDTH", &width,
         "Image Width (px)", &status) )
          return( status );

    // Write offset X //
    if ( fits_write_key(fptr, TLONG, "XOFFSET", &offsetX,
         "Image X Offset (px)", &status) )
         return( status );

    // Write offset Y //
    if ( fits_write_key(fptr, TLONG, "YOFFSET", &offsetY,
         "Image Y Offset (px)", &status) )
         return( status );


    fits_close_file(fptr, &status);            /* close the file */
    return( status );
}
