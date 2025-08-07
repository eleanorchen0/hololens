using System;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using UnityEngine;

public class client : MonoBehaviour
{
    private TcpClient client1;
    private NetworkStream stream;
    private Thread thread;
    private bool running;

    public GameObject EndPoint;
    public SineRope rope;

    private string ip = "10.29.224.211";
    private int port = 1984;

    Vector3 startPoint = new Vector3(0, 1.5f, 2);
    Quaternion rotation = Quaternion.identity;
    Vector3 endPoint = new Vector3(-0.5f, 1.5f, 2);

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
            (startPoint, rotation, endPoint) = ParseData(dataReceived);
        }
    }

    void OnApplicationQuit()
    {
        thread?.Abort();
    }

    public static (Vector3, Quaternion, Vector3) ParseData(string data)
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

        Vector3 pos1 = new Vector3(values[0], values[1] + 1.6f, values[2]);
        Quaternion rot1 = new Quaternion(values[3], values[4], values[5], values[6]);
        Vector3 pos2 = new Vector3(values[7], values[8] + 1.6f, values[9]);

        return (pos1, rot1, pos2);
    }

    void Update()
    {
        if (!float.IsNaN(startPoint.x))
        {
            rope.UpdateStart(startPoint);
            //transform.rotation = rotation1;
        }

        if (!float.IsNaN(endPoint.x))
        {
            EndPoint.transform.position = endPoint;
            rope.UpdateEnd(endPoint);
        }
    }
}