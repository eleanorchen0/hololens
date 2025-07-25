#------------------------------------------------------------------------------
# This script receives video from one of the HoloLens sideview grayscale
# cameras and plays it. The camera resolution is 640x480 @ 30 FPS. The stream 
# supports three operating modes: 0) video, 1) video + rig pose, 2) query 
# calibration (single transfer).
# Press esc to stop.
#------------------------------------------------------------------------------

from pynput import keyboard

import numpy as np
import cv2
import hl2ss_imshow
import hl2ss
import hl2ss_lnm
import hl2ss_utilities
import hl2ss_3dcv
import hl2ss_rus

# Settings --------------------------------------------------------------------

host = "10.29.211.183"

calibration_path = '/home/eleanor/Downloads/studying-main/hl2ss/calibration'
port = hl2ss.StreamPort.RM_VLC_LEFTFRONT
mode = hl2ss.StreamMode.MODE_1
profile = hl2ss.VideoProfile.H265_MAIN
bitrate = None

marker_length = 0.1

sphere_diameter = marker_length # in meters
texture_file = './texture.jpg'

aruco_dictionary = cv2.aruco.getPredefinedDictionary(cv2.aruco.DICT_6X6_250)
aruco_parameters = cv2.aruco.DetectorParameters()
aruco_detector = cv2.aruco.ArucoDetector(aruco_dictionary, aruco_parameters)
aruco_half = marker_length / 2
aruco_reference = np.array([[-aruco_half, aruco_half, 0], [aruco_half, aruco_half, 0], [aruco_half, -aruco_half, 0], [-aruco_half, -aruco_half, 0], [0, 0, sphere_diameter / 2]], dtype=np.float32)

listener = hl2ss_utilities.key_listener(keyboard.Key.esc)
listener.open()

ipc_unity = hl2ss_lnm.ipc_umq(host, hl2ss.IPCPort.UNITY_MESSAGE_QUEUE)
ipc_unity.open()

# Create textured sphere in Unity scene ---------------------------------------
with open(texture_file, mode='rb') as file:
    texture_data = file.read()

cb_unity = hl2ss_rus.command_buffer()
cb_unity.remove_all()
cb_unity.set_target_mode(hl2ss_rus.TargetMode.UseLast)
cb_unity.create_primitive(hl2ss_rus.PrimitiveType.Sphere)
cb_unity.set_texture(0, texture_data)

sphere_scale = [sphere_diameter, sphere_diameter, sphere_diameter]

ipc_unity.push(cb_unity)
ipc_unity.pull(cb_unity)


#------------------------------------------------------------------------------

client = hl2ss_lnm.rx_rm_vlc(host, port, mode=mode, profile=profile, bitrate=bitrate)
client.open()

while (not listener.pressed()):
    data = client.get_next_packet()
    image = data.payload.image


    calibration_vlc = hl2ss_3dcv.get_calibration_rm(calibration_path, host, port)

    intrinsics = calibration_vlc.intrinsics
    extrinsics = calibration_vlc.extrinsics

    corners, ids, _ = aruco_detector.detectMarkers(image)
    update_sphere = False

    if (corners and hl2ss.is_valid_pose(data.pose)):
        
        # Estimate aruco pose
        _, aruco_rvec, aruco_tvec = cv2.solvePnP(aruco_reference[:4, :], corners[0], intrinsics[:3, :3].transpose(), None, flags=cv2.SOLVEPNP_IPPE_SQUARE)
        aruco_R, _ = cv2.Rodrigues(aruco_rvec)
        aruco_pose = np.eye(4, 4, dtype=np.float32)
        aruco_pose[:3, :3] = aruco_R.transpose()
        aruco_pose[3, :3] = aruco_tvec.transpose()

        # Transform aruco corners to world coordinates
        aruco_to_world = aruco_pose @ hl2ss_3dcv.camera_to_rignode(extrinsics) @ hl2ss_3dcv.reference_to_world(data.pose)
        aruco_reference_world = hl2ss_3dcv.transform(aruco_reference, aruco_to_world)

        # Compute sphere position and rotation in Unity scene
        sphere_position = aruco_reference_world[4, :]

        sphere_rvec, _ = cv2.Rodrigues(aruco_to_world[:3, :3])
        sphere_angle = np.linalg.norm(sphere_rvec)
        sphere_axis = sphere_rvec / sphere_angle
        sphere_quaternion = np.vstack((sphere_axis * np.sin(sphere_angle / 2), np.array([[np.cos(sphere_angle / 2)]])))[:, 0]

        sphere_position[2] = -sphere_position[2]         # right hand to left hand 
        sphere_quaternion[2:3] = -sphere_quaternion[2:3] # coordinates conversion for Unity

        update_sphere = True

    # Update sphere state
    cb_unity = hl2ss_rus.command_buffer()
    if (update_sphere):
        cb_unity.set_world_transform(0, sphere_position, sphere_quaternion, sphere_scale)
        cb_unity.set_active(0, hl2ss_rus.ActiveState.Active)
    else:
        cb_unity.set_active(0, hl2ss_rus.ActiveState.Inactive)

    ipc_unity.push(cb_unity)


    image = cv2.aruco.drawDetectedMarkers(image, corners, ids)


    cv2.imshow('Video', image)
    cv2.waitKey(1)

cb_unity = hl2ss_rus.command_buffer()
cb_unity.remove_all()

ipc_unity.push(cb_unity)
ipc_unity.close()

client.close()
listener.close()
