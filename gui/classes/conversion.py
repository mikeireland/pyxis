
from datetime import datetime as dt
import numpy as np

#Latitude of Mt Stromlo
LAT = -35.321453
sinLAT = np.sin(np.radians(LAT))
cosLAT = np.cos(np.radians(LAT))

#Longitude of Mt Stromlo
LON = 149.006198
sinLON = np.sin(np.radians(LON))
cosLON = np.cos(np.radians(LON))

#J2000 offset
OFFSET = dt(2000,1,1,12)

Omega = 15.04106687606545


"""
Calculates current Greenwich Mean Siderial Time (GMST) in degrees
High accurate version (includes quadratic term)
"""
def GMST_HighAccuracy():
  now = dt.utcnow()
  jd = now - OFFSET

  #Julian Days since J2000
  D = jd.total_seconds()/86400

  #Current time in fractions of an hour
  H = now.hour + now.minute/60 + now.second/3600
  T = D/36525
  return (100.46061836999999 + 0.9856473662862001*D + 15*H + 0.000026*T**2)%360


"""
Calculates current Greenwich Mean Sidereal Time (GMST) in degrees
"""
def GMST():
  now = dt.utcnow()
  jd = now - OFFSET

  #Julian Days since J2000
  D = jd.total_seconds()/86400

  #Current time in fractions of an hour
  H = now.hour + now.minute/60 + now.second/3600
  return (100.46061836999999 + 0.9856473662862001*D + 15*H)%360


"""
Converts Celestial RA/DEC into Horizontal ALT/AZ coordinates
AZ is defined from North in a clockwise direction.

INPUTS
RA = Right ascension in degrees
DEC = Declination in degrees

OUTPUTS
ALT = Altitude in radians
AZ = Azimuth in radians

"""
def toAltAz_rad(RA,DEC):

  #Calc hour angle from Local Sidereal Time
  HA = (GMST() + LON - RA)%360
  radHA = np.radians(HA)
  radDEC = np.radians(DEC)

  sinALT = np.sin(radDEC)*sinLAT + np.cos(radDEC)*cosLAT*np.cos(radHA)
  ALT = np.arcsin(sinALT)

  cosA = (np.sin(radDEC) - sinALT*sinLAT)/(np.cos(ALT)*cosLAT)

  if np.sin(radHA) < 0:
      AZ = np.arccos(cosA)
  else:
      AZ = 2*np.pi - np.arccos(cosA)

  return ALT, AZ


"""
Converts celestial coordinates (Ra, Dec, Pos angle) into an AltAzPos
angles

INPUTS
RA = Right ascension in degrees
DEC = Declination in degrees
POS_ANG = Rotation of image from North Celestial Pole (invariant between
          celestial and horizontal coordinates)

OUTPUTS
AltAzPos angles
"""
def RaDec2AltAz(RA,DEC,POS_ANG):

    #Convert Ra/Dec to Altitude/Azimuth coordinates
    ALT, AZ = toAltAz_rad(RA,DEC)

    #Euler angles from the Alt/Az/Pos angle coordinates
    #Azimuth made to be anticlockwise in convention of Euler angles (Yaw).
    #Altitude equivalent to pitch

    #Pos angle equivalent to roll
    angles = (2*np.pi-AZ,ALT,np.radians(POS_ANG))

    return np.array(angles)

def RaDecDot(RA,DEC):
    #Convert Ra/Dec to Altitude/Azimuth coordinates
    ALT, AZ = toAltAz_rad(RA,DEC)

    dALTdt = Omega*np.sin(AZ)*cosLAT
    dAZdt = Omega*(sinLAT-(np.cos(AZ)*np.sin(ALT)*cosLAT)/np.cos(ALT))

    return dALTdt, dAZdt