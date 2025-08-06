using System;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using UnityEngine;
using MixedReality.Toolkit.UX;

public class client : MonoBehaviour
{
    private TcpClient client1;
    private NetworkStream stream;
    private Thread thread;
    private bool running;

    public GameObject EndPoint;

    private string ip = "10.29.224.211";
    private int port = 1984;

    Vector3 position1 = new Vector3(0, 1.5f, 2);
    Quaternion rotation1 = Quaternion.identity;

    Vector3 position2 = new Vector3(-0.5f, 1.5f, 2);
    Quaternion rotation2 = Quaternion.identity;

    void Start()
    {
        thread = new Thread(new ThreadStart(GetData));
        thread.Start();
    }

    void GetData()
    {
        client1 = new TcpClient(ip, port);
        Debug.Log("Connected");

        running = true;

        while (running)
        {
            Connect();
        }
    }

    void Connect()
    {
        stream = client1.GetStream();
        byte[] buffer = new byte[client1.ReceiveBufferSize];
        int bytesRead = stream.Read(buffer, 0, client1.ReceiveBufferSize);

        string dataReceived = Encoding.UTF8.GetString(buffer, 0, bytesRead);

        if (!string.IsNullOrEmpty(dataReceived))
        {
            (position1, rotation1, position2, rotation2) = ParseData(dataReceived);
        }
    }

    void OnApplicationQuit()
    {
        thread?.Abort();
    }

    public static (Vector3, Quaternion, Vector3, Quaternion) ParseData(string data)
    {
        string[] stringArray = data.Split(',');

        float[] values = new float[stringArray.Length];
        for (int i = 0; i < stringArray.Length; i++)
        {
            if (float.TryParse(stringArray[i], out float result))
                values[i] = result;
            else
                values[i] = float.NaN;
        }

        Vector3 pos1 = new Vector3(values[0], values[1] + 1.5f, values[2]);
        Quaternion rot1 = new Quaternion(values[3], values[4], values[5], values[6]);
        Vector3 pos2 = new Vector3(values[7], values[8] + 1.5f, values[9]);
        Quaternion rot2 = new Quaternion(values[10], values[11], values[12], values[13]);

        return (pos1, rot1, pos2, rot2);
    }

    void Update()
    {
        if (!float.IsNaN(position1.x))
        {
            transform.position = position1;
            transform.rotation = rotation1;
        }

        if (!float.IsNaN(position2.x))
        {
            EndPoint.transform.position = position2;
            EndPoint.transform.rotation = rotation2;
        }
    }
}