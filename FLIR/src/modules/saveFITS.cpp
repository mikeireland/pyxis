#include <iostream>
#include <string>
#include "acquisition.h"
#include "saveFITS.h"
#include "cpptoml/cpptoml.h"
#include "fitsio.h"

using namespace std;

/* Write a given vector array of image data as a FITS file
   INPUTS:
      config - cpptoml pointer to a configuration table
      fits_array - vector of image data to write
      times_struct - a structure that allows the storage of a timestamp and duration of the exposures

*/
int SaveFITS(std::shared_ptr<cpptoml::table> config, unsigned short* fits_array, Times times_struct)
{
    // Pointer to the FITS file; defined in fitsio.h
    fitsfile *fptr;

    // Define filepath and name for the FITS file
    std::string file_dir = config->get_qualified_as<std::string>("fits.file_dir").value_or("");
    std::string filename = config->get_qualified_as<std::string>("fits.filename").value_or("");
    string file_path = "!" + file_dir + filename;

    // Configure FITS file
    int width = config->get_qualified_as<int>("camera.width").value_or(0);
    int height = config->get_qualified_as<int>("camera.height").value_or(0);
    int bitpix = config->get_qualified_as<int>("fits.bitpix").value_or(8);
    long naxis = 3; // 2D image over time
    int buffer_size = config->get_qualified_as<int>("fits.buffer_size").value_or(0);
    long naxes[3] = {width, height, buffer_size};

    // Initialize status before calling fitsio routines
    int status = 0;

    // Create new FITS file. Will overwrite file with the same name!!
    if (fits_create_file(&fptr, file_path.c_str(), &status)){
       cout << "ERROR: Could not create FITS file" << endl;
       return( status );
    }

    // Write the required keywords for the primary array image
    if ( fits_create_img(fptr,  bitpix, naxis, naxes, &status) )
         return( status );

    long fpixel = 1;
    long nelements = naxes[0] * naxes[1] * naxes[2];

    // Write the image (assuming input of unsigned integers)
    if ( fits_write_img(fptr, TUSHORT, fpixel, nelements, fits_array, &status) ){
        cout << "ERROR: Could not write FITS file" << endl;
        return( status );
    }
    // Configure FITS header keywords

    int offset_x = buffer_size = config->get_qualified_as<long>("fits.buffer_size").value_or(0);
    int offset_y = config->get_qualified_as<int>("camera.offset_y").value_or(0);
    int exposure_time = config->get_qualified_as<int>("camera.exposure_time").value_or(0);

    cout << times_struct.timestamp << endl;

    // Write starting time in UTC
    if ( fits_write_key(fptr, TSTRING, "STARTTIME", &times_struct.timestamp,
         "Timestamp of beginning of exposure", &status) )
         return( status );

    // Write individual exposure time
    if ( fits_write_key(fptr, TINT, "FRAMEEXPOSURE", &exposure_time,
         "Individual Exposure Time (us)", &status) )
         return( status );

    // Write total exposure time
    if ( fits_write_key(fptr, TDOUBLE, "TOTALEXPOSURE", &times_struct.total_exposure,
         "Total Exposure Time (ms)", &status) )
         return( status );

    // Write height
    if ( fits_write_key(fptr, TINT, "HEIGHT", &height,
         "Image Height (px)", &status) )
         return( status );

    // Write width
    if ( fits_write_key(fptr, TINT, "WIDTH", &width,
         "Image Width (px)", &status) )
          return( status );

    // Write offset x
    if ( fits_write_key(fptr, TINT, "XOFFSET", &offset_x,
         "Image X Offset (px)", &status) )
         return( status );

    // Write offset y
    if ( fits_write_key(fptr, TINT, "YOFFSET", &offset_y,
         "Image Y Offset (px)", &status) )
         return( status );

    // Close file
    fits_close_file(fptr, &status);
    return( status );
}
