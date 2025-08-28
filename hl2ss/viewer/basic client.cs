using System;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using UnityEngine;
using UnityEngine.UIElements;
using TMPro;

public class basic_client : MonoBehaviour
{
    private TcpClient client;
    private NetworkStream stream;
    private Thread thread;
    private bool running;
    public GameObject gameObject;
    private string ip = "10.29.224.211";
    private int port = 1984;

    Vector3 position = new Vector3;

    void Start()
    {
        thread = new Thread(new ThreadStart(GetData));
        thread.Start();
    }

    void GetData()
    {
        client = new TcpClient(ip, port);
        Debug.Log("Connected");

        running = true;

        while (running)
        {
            Connect();
        }
    }

    void Connect()
    {
        stream = client.GetStream();
        byte[] buffer = new byte[client.ReceiveBufferSize];
        int bytesRead = stream.Read(buffer, 0, client.ReceiveBufferSize);

        string dataReceived = Encoding.UTF8.GetString(buffer, 0, bytesRead);

        if (dataReceived != null && dataReceived != "")
        {
            (position) = ParseData(dataReceived);
        }
    }

    void OnApplicationQuit()
    {
        thread?.Abort();
    }

    public static (Vector3) ParseData(string data)
    {
        Debug.Log(data);
        if (data.StartsWith("(") && data.EndsWith(")"))
        {
            data = data.Substring(1, data.Length - 2);
        }

        string[] stringArray = data.Split(',');

        Vector3 pos = new Vector3(
            float.Parse(stringArray[0]),
            float.Parse(stringArray[1]),
            float.Parse(stringArray[2]));
        Debug.Log(pos);

        return (pos);
    }

    private void Update()
    {
        gameObject.transform.position = position;
    }
}