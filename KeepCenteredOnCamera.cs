using UnityEngine;

public class KeepCenteredOnCamera : MonoBehaviour
{
    public Camera mainCamera;  
    public float distanceFromCamera = 8f;  
    public float xOffset = 1f;  // Compensar tamano de la mano, para centrar la imagen

    private void Start()
    {
        if (mainCamera == null)
        {
            mainCamera = Camera.main; 
        }
    }


    private void Update()
    {
        Vector3 cameraForward = mainCamera.transform.forward;  // Dirección en la que la cámara está mirando
        cameraForward.y = 0;  // Nos aseguramos de que no haya desplazamiento vertical

        // Normalizamos la dirección
        cameraForward.Normalize();

        // Colocamos el objeto a la distancia deseada en frente de la cámara
        Vector3 newPosition = mainCamera.transform.position + cameraForward * distanceFromCamera;

        newPosition += new Vector3(xOffset, 0, 0);
        // Actualizamos la posición del objeto
        transform.position = newPosition;
        //atte. chat gpt xd
    }
}
