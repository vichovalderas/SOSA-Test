using System;
using System.Net.WebSockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using UnityEngine;

public class WebSocketReceiver : MonoBehaviour
{
    private float timeLapse = 0f;
    private string wsUrl = "ws://mi-esp.local/ws"; //direccion privada del esp

    public Transform targetObject; //Game object del modelo de la mano
    
    private ClientWebSocket ws;
    private CancellationTokenSource cancelSource;

    Quaternion smoothQ;

    private async void Start()
    {
        cancelSource = new CancellationTokenSource();
        await ConnectWebSocket();
    }

    private async Task ConnectWebSocket()
    {
        while (!cancelSource.Token.IsCancellationRequested)
        {
            try //intenta una conexion mediante web socket con el esp
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

            Debug.Log("Reconectando en 3 segundos..."); //si falla, espera 3 segundos
            await Task.Delay(300);
        }
    }

    private async Task ReceiveLoop(CancellationToken token)
    {
        byte[] buffer = new byte[1024*8];

        while (ws.State == WebSocketState.Open && !token.IsCancellationRequested)//mientras la comunicacion se mantenga y no hayan instucciones para cerrarla
        {
            WebSocketReceiveResult result = null;
            try//se guarda en result un arreglo 'ArraySegment' de bytes del tamano buffer 
            {
                result = await ws.ReceiveAsync(new ArraySegment<byte>(buffer), token);
            }
            catch (Exception e)
            {
                Debug.LogWarning("WebSocket desconectado: " + e.Message);
                break; // Se sale del bucle y se reconecta
            }

            if (result == null || result.MessageType == WebSocketMessageType.Close)//si result es nulo o se ha mandado una peticion para cerrar la comunicacion, se rompe el ciclo
            {
                Debug.LogWarning("Conexión cerrada por el servidor.");
                break;
            }

            string message = Encoding.UTF8.GetString(buffer, 0, result.Count);
            try//message convierte el binario ArraySegment en texto
            {
                Quaternion q = ParseQuaternion(message);//llama a un metodo para parsear el json message a quaterniones para unity, luego lo guarda en el Quaternion q
                UnityMainThreadDispatcher.Instance().Enqueue(() => {
                    if (targetObject != null)//si existe la referencia para el gameobject de la mano, se le aplica la rotacion q
                        targetObject.localRotation = q;
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
