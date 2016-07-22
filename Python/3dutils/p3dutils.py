from numpy import matrix, array
import numpy as np
import cv2


from cv2 import Rodrigues


def interpolate(Fg_w1,Fg_w2,t):
    """Interpolates a rigid body motion proportionally between two endpoints.
    Makes the assumption of constant angular velocity, rather than dealing with pollhode motion stuff.

    Args:
        Fg_w1   (matrix): rigid body transform taking world coords to cg coordinates at t=0 [arraylike with shape (3x4) or (4x4)]
        Fg_w2   (matrix): rigid body transform taking world coords to cg coordinates at t=1 [arraylike with shape (3x4) or (4x4)]
        t        (Float): the time to interpolate to between transform 1 and transform 2

    Returns:
        Fg_wt   (matrix): rigid body transform taking world coords to cg coordinates at t=t (same type as transform 1)
    """
    F1 = matrix(Fg_w1)
    F2 = matrix(Fg_w2)

    if F1.shape == (4, 4) and F1[4, 4] != 1.0:
        temp = F1.copy()
        np.divide(temp,F1[4,4],temp)
        assert isinstance(temp, matrix)
        F1 = temp
    if F2.shape == (4, 4) and F2[4, 4] != 1.0:
        temp = F2.copy()
        np.divide(temp,F2[4,4],temp)
        assert isinstance(temp, matrix)
        F2 = temp

    R1 = F1[0:3,0:3]
    R2 = F2[0:3,0:3]

    Rd = R2*R1.T
    rd = array(Rodrigues(Rd)[0])
    Rt = matrix(Rodrigues(t*rd)[0])*R1

    T1 = F1[0:3,3:4]
    T2 = F2[0:3,3:4]

    Tt = T1*(1-t)+T2*t

    Ft = np.zeros_like(Fg_w1)
    Ft[-1,-1] = 1.0
    Ft[0:3,0:3] = Rt
    Ft[0:3,3:4] = Tt

    return Ft







