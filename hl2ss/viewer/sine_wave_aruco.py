#-----------------------------------------------------------------------------
import cv2
import hl2ss
import hl2ss_lnm
import numpy as np
import hl2ss_3dcv
import hl2ss_rus
import socket

# settings -------------------------------------------------------------------
# hl2ss.StreamPort.RM_VLC_LEFTFRONTs
# hl2ss.StreamPort.RM_VLC_LEFTLEFT
# hl2ss.StreamPort.RM_VLC_RIGHTFRONT
# hl2ss.StreamPort.RM_VLC_RIGHTRIGHT
hl2_port = hl2ss.StreamPort.RM_VLC_LEFTFRONT

hl2_host = "10.29.211.183"

host, port = "127.0.0.1", 1984

calibration_path = '/Users/elean/Documents/GitHub/studying-main/hl2ss/viewer/calibration'

# 0: video
# 1: video + rig pose
# 2: query calibration (single transfer)
mode = hl2ss.StreamMode.MODE_1

profile = hl2ss.VideoProfile.H265_MAIN
bitrate = None

# average position -----------------------------------------------------------
position = []
rotation = []

def average_time(time_position, current_time, time_interval):
    time_position[:] = [(time, position) for (time, position) in time_position if (current_time - time <= time_interval)]

    if not time_position:
        return None

    positions = np.array([position for (_, position) in time_position])
    average_position = positions.mean(axis=0)
    return average_position

# socket ---------------------------------------------------------------------
socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
try:
    socket.connect((host, port))
    print(f"Connected at {host}:{port}")

except Exception as e:
    print(f"Failed to connect at {host}:{port}: {e}")

# aruco ----------------------------------------------------------------------
marker_length  = 0.07

aruco_dict = cv2.aruco.getPredefinedDictionary(cv2.aruco.DICT_4X4_50)
aruco_parameters = cv2.aruco.DetectorParameters()
aruco_half = marker_length/2
aruco_reference = np.array([
    [-aruco_half, aruco_half, 0], 
    [aruco_half, aruco_half, 0], 
    [aruco_half, -aruco_half, 0], 
    [-aruco_half, -aruco_half, 0], 
    [0, 0, marker_length / 2]], dtype=np.float32)

#-----------------------------------------------------------------------------
client = hl2ss_lnm.rx_rm_vlc(hl2_host, hl2_port, mode=mode, profile=profile, bitrate=bitrate)
client.open()

while True:

    data = client.get_next_packet()
    time = data.timestamp
    
    frames = data.payload.image
    color_frames = cv2.cvtColor(frames, cv2.COLOR_GRAY2BGR)

    calibration_vlc = hl2ss_3dcv.get_calibration_rm(calibration_path, hl2_host, hl2_port)

    intrinsics = calibration_vlc.intrinsics
    extrinsics = calibration_vlc.extrinsics

    corners, ids, rejected = cv2.aruco.detectMarkers(color_frames, aruco_dict, parameters=aruco_parameters)

    update_wave = False

    if (ids is not None and hl2ss.is_valid_pose(data.pose)):

        # aruco coordinates --------------------------------------------------
        _, aruco_rotation_vec, aruco_translation_vec = cv2.solvePnP(aruco_reference[:4, :], corners[0], intrinsics[:3, :3].transpose(), None, flags=cv2.SOLVEPNP_IPPE_SQUARE)
        aruco_rotation, _ = cv2.Rodrigues(aruco_rotation_vec)

        aruco_pose = np.eye(4, 4, dtype = np.float32)
        aruco_pose[:3, :3] = aruco_rotation.transpose()
        aruco_pose[3, :3] = aruco_translation_vec.transpose()


        # corners to world coordinates
        aruco_to_world = aruco_pose @ hl2ss_3dcv.camera_to_rignode(extrinsics) @ hl2ss_3dcv.reference_to_world(data.pose)
        aruco_reference_world = hl2ss_3dcv.transform(aruco_reference, aruco_to_world)


        # wave start position ------------------------------------------------------
        updated_position = aruco_reference_world[4, :]

        wave_rotation_vec, _ = cv2.Rodrigues(aruco_to_world[:3, :3])
        wave_angle = np.linalg.norm(wave_rotation_vec)
        wave_axis = wave_rotation_vec / wave_angle
        updated_rotation = np.vstack((wave_axis * np.sin(wave_angle / 2), np.array([[np.cos(wave_angle / 2)]])))[:, 0]

        updated_position[2] = - updated_position[2]
        # updated_position[1] = updated_position[1] - 0.025 # (to be centered)
        updated_rotation[2:3] = - updated_rotation[2:3]

        update_wave = True

        cv2.aruco.drawDetectedMarkers(color_frames, corners, ids, (255,0,0))

        # averaging position for less noise ----------------------------------
        position.append([time, updated_position.copy()])
        rotation.append([time, updated_rotation.copy()])

        average_position = average_time(position, time, 5000000)
        average_rotation = average_time(rotation, time, 5000000)

        if update_wave and socket and average_position and average_rotation is not None:

            marker_data = {float(average_position[0]), 
                           float(average_position[1]), 
                           float(average_position[2]), 
                           float(average_rotation[0]), 
                           float(average_rotation[1]), 
                           float(average_rotation[2]), 
                           float(average_rotation[3])}
            
            try:
                socket.sendall(marker_data.encode('utf-8'))
                print("Sent")
                response = socket.recv(1024).decode("utf-8")
                print(response)
                
            except Exception as e:
                print(f"Failed to send : {e}")

    cv2.imshow("wave aruco", cv2.rotate(color_frames, cv2.ROTATE_90_CLOCKWISE))

    cv2.waitKey(1)

#-----------------------------------------------------------------------------
client.close()
socket.close()

cv2.destroyAllWindows()

#-----------------------------------------------------------------------------