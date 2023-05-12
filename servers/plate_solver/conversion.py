
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
Converts from yaw, pitch and roll Euler angles into a quaternion

INPUTS
r = list of yaw, pitch and roll angles in radians

OUTPUTS
Quaternion of the form [a,b,c,d] = ai + bj + ck + d
"""
def euler_to_quaternion(r):
    (yaw, pitch, roll) = (r[0], r[1], r[2])

    sinROLL = np.sin(roll/2)
    cosROLL = np.cos(roll/2)
    sinPITCH = np.sin(pitch/2)
    cosPITCH = np.cos(pitch/2)
    sinYAW = np.sin(yaw/2)
    cosYAW = np.cos(yaw/2)

    qx = sinROLL * cosPITCH * cosYAW - cosROLL * sinPITCH * sinYAW
    qy = cosROLL * sinPITCH * cosYAW + sinROLL * cosPITCH * sinYAW
    qz = cosROLL * cosPITCH * sinYAW - sinROLL * sinPITCH * cosYAW
    qw = cosROLL * cosPITCH * cosYAW + sinROLL * sinPITCH * sinYAW

    return [qx, qy, qz, qw]


"""
Converts celestial coordinates (Ra, Dec, Pos angle) into an AltAzPos
attitude quaternion

INPUTS
RA = Right ascension in degrees
DEC = Declination in degrees
POS_ANG = Rotation of image from North Celestial Pole (invariant between
          celestial and horizontal coordinates)

OUTPUTS
AltAzPos quaternion of the form [a,b,c,d] = ai + bj + ck + d
"""
def RaDec2Quat(RA,DEC,POS_ANG):

    #Convert Ra/Dec to Altitude/Azimuth coordinates
    ALT, AZ = toAltAz_rad(RA,DEC)

    #Euler angles from the Alt/Az/Pos angle coordinates
    #Azimuth made to be anticlockwise in convention of Euler angles (Yaw).
    #Altitude equivalent to pitch
    #Pos angle equivalent to roll
    angles = (2*np.pi-AZ,ALT,np.radians(POS_ANG))

    return np.array(euler_to_quaternion(angles))

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

"""
Converts celestial coordinates (Ra, Dec, Pos angle) and a target coordinate (Ra2,Dec2)
into AltAzPos differential angles

INPUTS
RA = Right ascension in degrees
DEC = Declination in degrees
POS_ANG = Rotation of image from North Celestial Pole (invariant between
          celestial and horizontal coordinates)
RA2 = Right ascension in degrees of target object
DEC2 = Right ascension in degrees of target object

OUTPUTS
AltAzPos angles
"""
def diffRaDec2AltAz(RA,DEC,POS_ANG,RA2,DEC2):

    #Convert Ra/Dec to Altitude/Azimuth coordinates
    ALT, AZ = toAltAz_rad(RA,DEC)

    #Convert Ra/Dec to Altitude/Azimuth coordinates for second object
    ALT2, AZ2 = toAltAz_rad(RA2,DEC2)

    dALT = ALT2-ALT
    dAZ = AZ2 - AZ

    #Euler angles from the Alt/Az/Pos angle coordinates
    #Azimuth made to be anticlockwise in convention of Euler angles (Yaw).
    #Altitude equivalent to pitch

    #Pos angle equivalent to roll
    angles = (2*np.pi-dAZ,dALT,np.radians(POS_ANG))

    return np.array(angles)