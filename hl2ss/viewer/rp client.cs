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

    private string ip = "10.29.224.211";
    private int port = 1984;

    Vector3 position1 = Vector3.zero;
    Vector3 position2 = Vector3.zero;
    Vector3 position3 = Vector3.zero;
    Quaternion rotation = Quaternion.identity;

    Quaternion parsedRotation = Quaternion.identity;

    public DipoleAntenna d;
    public PatchAntenna p;
    public HornAntenna h;
    public GameObject rotator;

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
        Debug.Log("connecting");
        stream = client1.GetStream();
        byte[] buffer = new byte[client1.ReceiveBufferSize];
        int bytesRead = stream.Read(buffer, 0, client1.ReceiveBufferSize);

        string dataReceived = Encoding.UTF8.GetString(buffer, 0, bytesRead);

        if (!string.IsNullOrEmpty(dataReceived))
        {
            (position1, position2, position3, parsedRotation) = ParseData(dataReceived);

            // Only update rotation if it's valid
            if (!IsInvalidQuaternion(parsedRotation))
            {
                rotation = parsedRotation;
            }

            Debug.Log((position1, position2, position3, rotation));
        }
    }

    void OnApplicationQuit()
    {
        thread?.Abort();
    }

    public static (Vector3, Vector3, Vector3, Quaternion) ParseData(string data)
    {
        Debug.Log("parsing data");
        string[] stringArray = data.Split(',');

        float[] values = new float[stringArray.Length];
        for (int i = 0; i < stringArray.Length; i++)
        {
            if (float.TryParse(stringArray[i], out float result))
                values[i] = result;
            else
                values[i] = float.NaN;
        }

        Vector3 pos1 = new Vector3(values[0] + 2f, values[1] + 1.6f, values[2] - 1.5f);
        Vector3 pos2 = new Vector3(values[3] + 2f, values[4] + 1.6f, values[5] - 1.5f);
        Vector3 pos3 = new Vector3(values[6] + 2.05f, values[7] + 1.54f, values[8] - 1.5f);
        Quaternion rot = new Quaternion(values[9], values[10], values[11], values[12]);

        return (pos1, pos2, pos3, rot);
    }

    bool IsInvalidQuaternion(Quaternion q)
    {
        return Mathf.Approximately(q.x, 0f) &&
               Mathf.Approximately(q.y, 0f) &&
               Mathf.Approximately(q.z, 0f) &&
               Mathf.Approximately(q.w, 0f);
    }

    void Update()
    {
        if (!float.IsNaN(position1.x))
        {
            d.transform.position = position1;
        }
        if (!float.IsNaN(position2.x))
        {
            p.transform.position = position2;
        }
        if (!float.IsNaN(position3.x))
        {
            h.transform.position = position3;
        }

        // Smooth rotation update
        if (!IsInvalidQuaternion(rotation))
        {
            rotator.transform.rotation = Quaternion.Slerp(rotator.transform.rotation, rotation, Time.deltaTime * 5f);

            Vector3 current = rotator.transform.eulerAngles;
            Vector3 newrot = new Vector3(0, current.y, 0);
            //h.transform.eulerAngles = newrot;
        }
    }
}