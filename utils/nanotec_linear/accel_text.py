"""
A test program to read in the accelerometers when output as text.

Linear actuator: At a 3ms period, there is a 0.008g amplitude acceleration. This
corresponds to an 18nm oscillation. Definitely seems smooth enough.
"""
import numpy as np
import matplotlib.pyplot as plt
lsb_g = 16000.0

