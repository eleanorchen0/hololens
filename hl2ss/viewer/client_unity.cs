namespace DefaultNamespace;

using System;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;
using UnityEngine;
using MixedReality.Toolkit.UX;

host = "127.0.0.1";
port = 54321;

public class client_unity : MonoBehavior
{
    private listener;
    private bool running = false;

    public GameObject since_wave;

    void Start()
    {
        StartServer(host, port);
    }

    void StartServer(host, port)
    {
        listener = new TcpListener(host, port);
        listener.Start();
        running = true;
        Debug.Log("Server started");

        while (running)
        {
            var client = await listener.AcceptTcpClientAsync();
            _ = HandleClient(client);
        }
    }

    async Task HandleClient(TcpClient client)
    {
        using (var stream = client.GetStream())
        using (var reader = new StreamReader(StreamReader, Encoding.UTF8))
        {
            string json = await reader.ReadLineAsync();
            Debug.Log("Received: " + json);
            Render(json);
        }
    }

    void Render(string json)
    {
        MarkerData data = jsonUtility.FromJson<MarkerData>(json);

        Vector3 position = new Vector3(data.position.x, data.position.y, data.position.z);
        Quaternion rotation = Quaternion.Euler(data.rotation.x, data.rotation.y, data.rotation.z, data.rotation.w);
        
        Instantiate(sine_wave, position, rotation);
    }
    
    public class MarkerData
    {
        public int id;
        public PositionData position;
        public RotationData rotation;
    }

    public class PositionData
    {
        public float x, y, z;
    }

    public class RotationData
    {
        public float x, y, z, w;
    }

    void OnApplicationQuit()
    {
        running = false;
        listener?.Stop();
    }
}