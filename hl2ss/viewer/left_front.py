import cv2
import hl2ss_imshow
import hl2ss
import hl2ss_lnm
import hl2ss_utilities
import hl2ss_3dcv
import numpy as np
import socket
import hl2ss_rus


calibration_path = 'calibration'
port = hl2ss.StreamPort.RM_VLC_LEFTFRONT
host = "10.29.211.183"

calibration_vlc = hl2ss_3dcv.get_calibration_rm(calibration_path, host, port)

intrinsics = calibration_vlc.intrinsics
extrinsics = calibration_vlc.extrinsics