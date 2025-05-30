using System;
using System.Net.WebSockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using UnityEngine;

public class WebSocketReceiver : MonoBehaviour
{
    private string wsUrl = "ws://192.168.163.152/ws"; // Cambia esto a la IP real
    public Transform targetObject;
    
    private ClientWebSocket ws;
    private CancellationTokenSource cancelSource;

    private async void Start()
    {
        cancelSource = new CancellationTokenSource();
        await ConnectWebSocket();
    }

    private async Task ConnectWebSocket()
    {
        while (!cancelSource.Token.IsCancellationRequested)
        {
            try
            {
                ws = new ClientWebSocket();
                Debug.Log("Conectando a WebSocket...");
                await ws.ConnectAsync(new Uri(wsUrl), cancelSource.Token);
                Debug.Log("Conexión establecida.");

                await ReceiveLoop(cancelSource.Token);
            }
            catch (Exception e)
            {
                Debug.LogError("WebSocket error: " + e.Message);
            }

            Debug.Log("Reconectando en 3 segundos...");
            await Task.Delay(5);
        }
    }

    private async Task ReceiveLoop(CancellationToken token)
    {
        byte[] buffer = new byte[1024*8];

        while (ws.State == WebSocketState.Open && !token.IsCancellationRequested)
        {
            WebSocketReceiveResult result = null;
            try
            {
                result = await ws.ReceiveAsync(new ArraySegment<byte>(buffer), token);
            }
            catch (Exception e)
            {
                Debug.LogWarning("WebSocket desconectado: " + e.Message);
                break; // Se sale del bucle y se reconecta
            }

            if (result == null || result.MessageType == WebSocketMessageType.Close)
            {
                Debug.LogWarning("Conexión cerrada por el servidor.");
                break;
            }

            string message = Encoding.UTF8.GetString(buffer, 0, result.Count);
            try
            {
                Quaternion q = ParseQuaternion(message);
                UnityMainThreadDispatcher.Instance().Enqueue(() => {
                    if (targetObject != null)
                        targetObject.localRotation = Quaternion.Slerp(targetObject.localRotation,q,60);
                });
            }
            catch (Exception e)
            {
                Debug.LogWarning("Error al parsear: " + e.Message);
            }
        }
    }

    private Quaternion ParseQuaternion(string json)
    {
        float qx = GetValue(json, "qx");
        float qy = GetValue(json, "qy");
        float qz = GetValue(json, "qz");
        float qw = GetValue(json, "qw");
        return new Quaternion(qx, qy, qz, qw);
    }

    private float GetValue(string json, string key)
    {
        int i = json.IndexOf(key);
        if (i == -1) return 0;
        int start = json.IndexOf(':', i) + 1;
        int end = json.IndexOf(',', start);
        if (end == -1) end = json.IndexOf('}', start);
        string val = json.Substring(start, end - start);
        return float.Parse(val);
    }

    private void OnApplicationQuit()
    {
        cancelSource.Cancel();
        ws?.Abort();
        ws?.Dispose();
    }
}
