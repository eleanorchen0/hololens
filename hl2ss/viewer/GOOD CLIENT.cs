using System;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using Unity.VisualScripting;
using UnityEngine;
using UnityEngine.UIElements;

public class client : MonoBehaviour
{
    private TcpClient client1;
    private NetworkStream stream;
    private Thread thread;
    private bool running;

    private string ip = "10.29.224.211";
    private int port = 1984;

    [Header("Connect to waves")]
    public GameObject EndPoint;
    public SineRope rope;
    public ReflectedRope reflection;
    public TransmittedRope transmission;
    public float material;

    [Header("Directional Control")]
    public Vector3 startPoint = new Vector3(0, 1.5f, 2);
    public Vector3 endPoint = new Vector3(1.5f, 1.5f, 2);
    public float angle = 0;

    [Header("Reflection")]
    public float reflected = 1;
    public float absorbed;

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
            (startPoint, endPoint, material) = ParseData(dataReceived);
            if (material == 0)
            {
                reflected = 1.00f;
            }
            else if (material == 3) { reflected = 0.10f; absorbed = 0.50f; }
        }
    }

    void OnApplicationQuit()
    {
        thread?.Abort();
    }

    public static (Vector3, Vector3, float) ParseData(string data)
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
        Vector3 pos2 = new Vector3(values[3], values[4] + 1.6f, values[5]);
        float mat = values[6];


        return (pos1, pos2, mat);
    }

    public float CalculateAngle(Vector3 pos1, Vector3 pos2)
    {
        return Mathf.Atan2(pos2.y - pos1.y, Mathf.Abs(pos2.x - pos1.x)) * Mathf.Rad2Deg;
    }

    void Update()
    {

        if (!float.IsNaN(startPoint.x))
        {
            rope.UpdateStart(startPoint);
        }

        if (!float.IsNaN(endPoint.x))
        {
            EndPoint.transform.position = endPoint;
            rope.UpdateEnd(endPoint);
        }
        angle = Mathf.Abs(CalculateAngle(endPoint, startPoint));

    }
}