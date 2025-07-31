import cv2
import hl2ss_imshow
import hl2ss
import hl2ss_lnm
import hl2ss_utilities
import numpy as np
import hl2ss_3dcv
import hl2ss_rus
import socket


host = "10.29.211.183"

calibration_path = 'calibration'

port = hl2ss.StreamPort.RM_VLC_RIGHTFRONT
mode = hl2ss.StreamMode.MODE_2
profile = hl2ss.VideoProfile.H265_MAIN
bitrate = None
unity_host, unity_port = "127.0.0.1", 1984

# position = []
# rotation = []
# 
# def average_time(time_position, current_time, time_interval):
#     time_position[:] = [(time, position) for (time, position) in time_position if (current_time - time <= time_interval)]
# 
#     if not time_position:
#         return None
# 
#     positions = np.array([position for (_, position) in time_position])
#     average_position = positions.mean(axis=0)
#     return average_position
# marker_length  = 0.07
# 
# aruco_dict = cv2.aruco.getPredefinedDictionary(cv2.aruco.DICT_4X4_50)
# aruco_parameters = cv2.aruco.DetectorParameters()
# aruco_half = marker_length/2
# aruco_reference = np.array([[-aruco_half, aruco_half, 0], [aruco_half, aruco_half, 0], [aruco_half, -aruco_half, 0], [-aruco_half, -aruco_half, 0], [0, 0, marker_length / 2]], dtype=np.float32)


client = hl2ss_lnm.rx_rm_vlc(host, port, mode=mode, profile=profile, bitrate=bitrate)
client.open()

while True:
    data = client.get_next_packet()
    time = data.timestamp

    frames = data.payload.image
    color_frames = cv2.cvtColor(frames, cv2.COLOR_GRAY2BGR)

    cv2.imshow('Video', data.payload.image)
    cv2.waitKey(1)

client.close()
