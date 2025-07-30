using System;
using System.Net;
using System.Net.Sockets;
using System.Text;

class EchoServer
{
    static void Main()
    {
        string host = "10.29.224.211";
        int port = 65432;

        IPAddress ipAddress = IPAddress.Parse(host);
        IPEndPoint localEndPoint = new IPEndPoint(ipAddress, port);

        TcpListener server = new TcpListener(localEndPoint);

        server.Start();
        Console.WriteLine($"Listening on {host}:{port}");

        TcpClient client = server.AcceptTcpClient();
        Console.WriteLine("Connected to " + client.Client.RemoteEndPoint);

        NetworkStream stream = client.GetStream();
        byte[] buffer = new byte[1024];

        while (true)
        {
            int bytesRead = stream.Read(buffer, 0, buffer.Length);
            if (bytesRead == 0)
                break;

            stream.Write(buffer, 0, bytesRead);
        }

        client.Close();
        server.Stop();
    }
}