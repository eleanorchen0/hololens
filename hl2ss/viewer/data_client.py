#------------------------------------------------------------------------------
import cv2
import hl2ss_imshow
import hl2ss
import hl2ss_lnm
import hl2ss_utilities
import numpy as np
import hl2ss_3dcv
import hl2ss_rus
import socket

# settings --------------------------------------------------------------------
host = "10.29.211.183"
port = hl2ss.StreamPort.RM_VLC_LEFTFRONT

calibration_path = "calibration"

mode = hl2ss.StreamMode.MODE_1
profile = hl2ss.VideoProfile.H265_MAIN
bitrate = None

#------------------------------------------------------------------------------
unity_host, unity_port = "127.0.0.1", 1984

# socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# try:
#     socket.connect((unity_host, unity_port))
#     print(f"Connected at {host}:{port}")
# except Exception as e:
#     print(f"Failed to connect to {host}")

#------------------------------------------------------------------------------
position = []
rotation = []

def average_time(time_position, current_time, time_interval):
    time_position[:] = [(time, position) for (time, position) in time_position if (current_time - time <= time_interval)]

    if not time_position:
        return None

    positions = np.array([position for (_, position) in time_position])
    average_position = positions.mean(axis=0)
    return average_position

# aruco -----------------------------------------------------------------------
marker_length  = 0.07

aruco_dict = cv2.aruco.getPredefinedDictionary(cv2.aruco.DICT_4X4_50)
aruco_parameters = cv2.aruco.DetectorParameters()
aruco_half = marker_length/2
aruco_reference = np.array([[-aruco_half, aruco_half, 0], [aruco_half, aruco_half, 0], [aruco_half, -aruco_half, 0], [-aruco_half, -aruco_half, 0], [0, 0, marker_length / 2]], dtype=np.float32)

# wave ------------------------------------------------------------------------
scale = [marker_length, marker_length, marker_length]
key = 0

client = hl2ss_lnm.rx_rm_vlc(host, port, mode=mode, profile=profile, bitrate=bitrate)
client.open()

# main loop -------------------------------------------------------------------
while True:

    data = client.get_next_packet()
    time = data.timestamp

    frames = data.payload.image
    color_frames = cv2.cvtColor(frames, cv2.COLOR_GRAY2BGR)

    calibration_vlc = hl2ss_3dcv.get_calibration_rm(calibration_path, host, port)

    intrinsics = calibration_vlc.intrinsics
    extrinsics = calibration_vlc.extrinsics

    corners, ids, rejected = cv2.aruco.detectMarkers(color_frames, aruco_dict, parameters=aruco_parameters)

    update= False

    if (ids is not None and hl2ss.is_valid_pose(data.pose)):

        # aruco coordinates
        _, aruco_rotation_vec, aruco_translation_vec = cv2.solvePnP(aruco_reference[:4, :], corners[0], intrinsics[:3, :3].transpose(), None, flags=cv2.SOLVEPNP_IPPE_SQUARE)
        aruco_rotation, _ = cv2.Rodrigues(aruco_rotation_vec)

        aruco_pose = np.eye(4, 4, dtype = np.float32)
        aruco_pose[:3, :3] = aruco_rotation.transpose()
        aruco_pose[3, :3] = aruco_translation_vec.transpose()

        # corners to world coordinates
        aruco_to_world = aruco_pose @ hl2ss_3dcv.camera_to_rignode(extrinsics) @ hl2ss_3dcv.reference_to_world(data.pose)
        aruco_reference_world = hl2ss_3dcv.transform(aruco_reference, aruco_to_world)

        # position
        updated_position = aruco_reference_world[4, :]
        rotation_vec, _ = cv2.Rodrigues(aruco_to_world[:3, :3])
        angle = np.linalg.norm(rotation_vec)
        axis = rotation_vec / angle
        updated_rotation = np.vstack((axis * np.sin(angle / 2), np.array([[np.cos(angle / 2)]])))[:, 0]

        # convert for unity
        updated_position[2] = - updated_position[2]
        updated_rotation[2:3] = - updated_rotation[2:3]

        # print(updated_position, updated_rotation)

        cv2.aruco.drawDetectedMarkers(color_frames, corners, ids, (0,255,0))

        position.append([time, updated_position.copy()])
        rotation.append([time, updated_rotation.copy()])

        average_position = average_time(position, time, 5000000)
        average_rotation = average_time(rotation, time, 5000000)

        update = True

    # update 

    if update:

        d = {float(average_position[0]), 
            float(average_position[1]), 
            float(average_position[2]), 
            float(average_rotation[0]), 
            float(average_rotation[1]), 
            float(average_rotation[2]), 
            float(average_rotation[3])}

        print(d)

        # try:
        #     socket.sendall(d.encode("utf-8"))
        #     print(f"Sent {d}")
        #     response = socket.recv(1024).decode("utf-8")


    cv2.imshow("wave aruco", cv2.rotate(color_frames, cv2.ROTATE_90_CLOCKWISE))

    cv2.waitKey(1)

#------------------------------------------------------------------------------
client.close()

#------------------------------------------------------------------------------