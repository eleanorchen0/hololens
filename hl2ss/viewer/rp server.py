#------------------------------------------------------------------------------
import cv2
import hl2ss
import hl2ss_lnm
import hl2ss_3dcv
import numpy as np
import socket

# settings --------------------------------------------------------------------
host = "10.29.211.183"
port = hl2ss.StreamPort.RM_VLC_RIGHTFRONT

calibration_path = "calibration"

mode = hl2ss.StreamMode.MODE_1
profile = hl2ss.VideoProfile.H265_MAIN
bitrate = None

# ------------------------------------------------------------------------------
unity_host, unity_port = "0.0.0.0", 1984

server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind((unity_host, unity_port))
server_socket.listen(1)

print(f"Python Server listening on {unity_host}:{unity_port}")
conn, addr = server_socket.accept()
print(f"Connection from {addr}")

#------------------------------------------------------------------------------
# 1 : dipole
# 2 : patch
# 3 : horn 
position_dict = {1:[], 2:[], 3:[]}
rotation_dict = {1:[], 2:[], 3:[]}
def average_time(time_position, current_time, time_interval):
    time_position[:] = [(time, position) for (time, position) in time_position if (current_time - time <= time_interval)]

    if not time_position:
        return None

    positions = np.array([position for (_, position) in time_position])
    return positions.mean(axis=0)

def format_vector(v):
    return ','.join(str(float(x)) if x is not None else 'NaN' for x in v)

# aruco -----------------------------------------------------------------------
marker_length  = 0.06

aruco_dict = cv2.aruco.getPredefinedDictionary(cv2.aruco.DICT_4X4_50)
aruco_parameters = cv2.aruco.DetectorParameters()
aruco_half = marker_length/2
aruco_reference = np.array([[-aruco_half, aruco_half, 0], [aruco_half, aruco_half, 0], [aruco_half, -aruco_half, 0], [-aruco_half, -aruco_half, 0], [0, 0, marker_length / 2]], dtype=np.float32)

# wave ------------------------------------------------------------------------
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

    if (ids is not None and hl2ss.is_valid_pose(data.pose)):
        for i, marker_id in enumerate(ids.flatten()):
            if marker_id not in [1,2,3]:
                continue

            # aruco coordinates
            _, aruco_rotation_vec, aruco_translation_vec = cv2.solvePnP(aruco_reference[:4, :], corners[i], intrinsics[:3, :3].transpose(), None, flags=cv2.SOLVEPNP_IPPE_SQUARE)
            aruco_rotation, _ = cv2.Rodrigues(aruco_rotation_vec)

            aruco_pose = np.eye(4, 4, dtype = np.float32)
            aruco_pose[:3, :3] = aruco_rotation.transpose()
            aruco_pose[3, :3] = aruco_translation_vec.transpose()

            # corners to world coordinates
            aruco_to_world = aruco_pose @ hl2ss_3dcv.camera_to_rignode(extrinsics) @ hl2ss_3dcv.reference_to_world(data.pose)
            aruco_reference_world = hl2ss_3dcv.transform(aruco_reference, aruco_to_world)

            # position
            updated_position = aruco_reference_world[4, :]

            #rotation
            rotation_vec, _ = cv2.Rodrigues(aruco_to_world[:3, :3])
            angle = np.linalg.norm(rotation_vec)
            axis = rotation_vec / angle
            updated_rotation = np.vstack((axis * np.sin(angle / 2), np.array([[np.cos(angle / 2)]])))[:, 0]

            # convert for unity
            updated_position[2] = - updated_position[2]
            updated_rotation[2:3] = - updated_rotation[2:3]

            position_dict[marker_id].append([time, updated_position.copy()])
            rotation_dict[marker_id].append([time, updated_rotation.copy()])

        cv2.aruco.drawDetectedMarkers(color_frames, [corners[i]], np.array([marker_id]), (0,255,0))

        dipole = average_time(position_dict[1], time, 25000000)
        patch = average_time(position_dict[2], time, 25000000)
        horn = average_time(position_dict[3], time, 25000000)
        rot = average_time(rotation_dict[3], time, 25000000)

        dipole_pos = format_vector(dipole if dipole is not None else [None, None, None])
        patch_pos = format_vector(patch if patch is not None else [None, None, None])
        horn_pos = format_vector(horn if horn is not None else [None, None, None])
        horn_rot = format_vector(rot if rot is not None else [None, None, None, None])

        d = f"{dipole_pos}, {patch_pos}, {horn_pos}, {horn_rot}"
        print(d)
        conn.sendall(d.encode('utf-8'))

    cv2.imshow("wave aruco", cv2.rotate(color_frames, cv2.ROTATE_90_COUNTERCLOCKWISE))
    cv2.waitKey(1)

#------------------------------------------------------------------------------
server_socket.close()
conn.close()
client.close()
cv2.destroyAllWindows()
#------------------------------------------------------------------------------