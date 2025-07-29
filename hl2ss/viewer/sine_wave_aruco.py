#-----------------------------------------------------------------------------
import cv2
import hl2ss
import hl2ss_lnm
import numpy as np
import hl2ss_3dcv
import hl2ss_rus
import socket

# settings -------------------------------------------------------------------
host = "127.0.0.1" # pv still doesn't work

# hl2ss.StreamPort.RM_VLC_LEFTFRONT
# hl2ss.StreamPort.RM_VLC_LEFTLEFT
# hl2ss.StreamPort.RM_VLC_RIGHTFRONT
# hl2ss.StreamPort.RM_VLC_RIGHTRIGHT
port = hl2ss.StreamPort.RM_VLC_LEFTFRONT

unity_ip = "10.29.211.183"
unity_port = 65432
unity_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
unity_socket.connect((unity_ip, unity_port))

calibration_path = '/home/eleanor/Downloads/studying-main/hl2ss/calibration'

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
def connect_to_server(host, port):
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect(host, port)
            print(f"Connected to server at {host}:{port}")

            message = marker_data
            s.sendall(message.encode('utf-8'))
            print(f"Sent {message}")

            while True:
                data = s.recv(1024)
                if not data:
                    break
                print("Received from server:", data.decode('utf-8'))

    except ConnectionRefusedError:
        print("Could not connect to server")
    except Exception as e:
        print(e)

# aruco ----------------------------------------------------------------------
marker_length  = 0.07

aruco_dict = cv2.aruco.getPredefinedDictionary(cv2.aruco.DICT_4X4_50)


aruco_parameters = cv2.aruco.DetectorParameters()
aruco_half = marker_length/2
aruco_reference = np.array([[-aruco_half, aruco_half, 0], [aruco_half, aruco_half, 0], [aruco_half, -aruco_half, 0], [-aruco_half, -aruco_half, 0], [0, 0, marker_length / 2]], dtype=np.float32)

# unity ----------------------------------------------------------------------
ipc_unity = hl2ss_lnm.ipc_umq(unity_ip, hl2ss.IPCPort.UNITY_MESSAGE_QUEUE)
ipc_unity.open()

# wave -----------------------------------------------------------------------
command_buffer = hl2ss_rus.command_buffer()

command_buffer.begin_display_list()
command_buffer.remove_all()
command_buffer.set_target_mode(hl2ss_rus.TargetMode.UseLast)
command_buffer.set_target_mode(hl2ss_rus.TargetMode.UseID)
ipc_unity.push(command_buffer)
results = ipc_unity.pull(command_buffer)
key = results[2]

print(f'Created wave with id {key}')

#-----------------------------------------------------------------------------
client = hl2ss_lnm.rx_rm_vlc(unity_ip, port, mode=mode, profile=profile, bitrate=bitrate)
client.open()

while True:

    data = client.get_next_packet()
    time = data.timestamp
    print(time)

    frames = data.payload.image
    color_frames = cv2.cvtColor(frames, cv2.COLOR_GRAY2BGR)

    calibration_vlc = hl2ss_3dcv.get_calibration_rm(calibration_path, unity_ip, port)

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

    # update wave
    command_buffer = hl2ss_rus.command_buffer()

    if update_wave:
        
        marker_data = {
            "id" : ids,
            "position" : {"x" : average_position[0], "y" : average_position[1], "z" : average_position[2]},
            "rotation" : {"x" : average_rotation[0], "y" : average_rotation[1], "z" : average_rotation[2], "w" : average_rotation[3]},
        }

        connect_to_server(unity_ip, unity_port)

    cv2.imshow("wave aruco", cv2.rotate(color_frames, cv2.ROTATE_90_CLOCKWISE))

    cv2.waitKey(1)

#-----------------------------------------------------------------------------
client.close()

command_buffer = hl2ss_rus.command_buffer()
command_buffer.remove(key)

ipc_unity.push(command_buffer)
results = ipc_unity.pull(command_buffer)

ipc_unity.close()

#-----------------------------------------------------------------------------