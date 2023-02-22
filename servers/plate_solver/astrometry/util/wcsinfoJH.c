/*
 # This file is part of the Astrometry.net suite.
 # Licensed under a 3-clause BSD style license - see LICENSE
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include "os-features.h"
#include "sip.h"
#include "sip-utils.h"
#include "sip_qfits.h"
#include "starutil.h"
#include "mathutil.h"
#include "boilerplate.h"
#include "errors.h"
#include "wcsinfoJH.h"

const char* OPTIONS = "he:W:H:";

int get_WCS_info(char* filename, double *ra, double *dec, double *ori) {
    int argchar;
    char* progname = args[0];
    char** inputfiles = NULL;
    int ninputfiles = 0;
    int ext = 0;
    sip_t wcs;
    double imw=0, imh=0;
    double rac, decc;
    double det, parity, orient, orientc;
    int rah, ram, decsign, decd, decm;
    double ras, decs;
    char* units;
    double pixscale;
    double fldw, fldh;
    double ramin, ramax, decmin, decmax;
    double mxlo, mxhi, mylo, myhi;
    double dm;
    int merczoom;
    char rastr[32];
    char decstr[32];

    if (!sip_read_header_file_ext(filename, ext, &wcs)) {
        ERROR("failed to read WCS header from file %s, extension %i", inputfiles[0], ext);
        return -1;
    }

    if (imw == 0)
        imw = wcs.wcstan.imagew;
    if (imh == 0)
        imh = wcs.wcstan.imageh;
    if ((imw == 0.0) || (imh == 0.0)) {
        ERROR("failed to find IMAGE{W,H} in WCS file");
        return -1;
    }
    // If W,H were set on the cmdline...
    if (wcs.wcstan.imagew == 0)
        wcs.wcstan.imagew = imw;
    if (wcs.wcstan.imageh == 0)
        wcs.wcstan.imageh = imh;

    det = sip_det_cd(&wcs);
    parity = (det >= 0 ? 1.0 : -1.0);
    pixscale = sip_pixel_scale(&wcs);
    printf("det %.12g\n", det);
    printf("parity %i\n", (int)parity);
    printf("pixscale %.12g\n", pixscale);

    sip_get_radec_center(&wcs, &rac, &decc);
    printf("ra_center %.12g\n", rac);
    printf("dec_center %.12g\n", decc);

    orient = sip_get_orientation(&wcs);
    printf("orientation %.8g\n", orient);

    // contributed by Rob Johnson, user rob at the domain whim.org, Nov 13, 2009
    orientc = orient + rad2deg(atan(tan(deg2rad(rac - wcs.wcstan.crval[0])) * sin(deg2rad(wcs.wcstan.crval[1]))));
    printf("orientation_center %.8g\n", orientc);

    fldw = imw * pixscale;
    fldh = imh * pixscale;
    // area of the field, in square degrees.
    printf("fieldarea %g\n", (arcsec2deg(fldw) * arcsec2deg(fldh)));

    sip_get_field_size(&wcs, &fldw, &fldh, &units);
    printf("fieldw %.4g\n", fldw);
    printf("fieldh %.4g\n", fldh);
    printf("fieldunits %s\n", units);

    sip_get_radec_bounds(&wcs, 10, &ramin, &ramax, &decmin, &decmax);
    printf("decmin %g\n", decmin);
    printf("decmax %g\n", decmax);
    printf("ramin %g\n", ramin);
    printf("ramax %g\n", ramax);

    *ra = rac;
    *dec = decc;
    *ori = orient;

    return 0;
}
