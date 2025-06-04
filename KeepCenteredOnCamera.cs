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
        Vector3 cameraForward = mainCamera.transform.forward;  // Direcci�n en la que la c�mara est� mirando
        cameraForward.y = 0;  // Nos aseguramos de que no haya desplazamiento vertical

        // Normalizamos la direcci�n
        cameraForward.Normalize();

        // Colocamos el objeto a la distancia deseada en frente de la c�mara
        Vector3 newPosition = mainCamera.transform.position + cameraForward * distanceFromCamera;

        newPosition += new Vector3(xOffset, 0, 0);
        // Actualizamos la posici�n del objeto
        transform.position = newPosition;
        //atte. chat gpt xd
    }
}
