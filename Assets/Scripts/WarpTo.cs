
using UdonSharp;
using UnityEngine;
using VRC.SDKBase;
using VRC.Udon;

public class WarpTo : UdonSharpBehaviour
{
	public GameObject Destination;
	
	public GameObject ToggleOff0;
	public GameObject ToggleOff1;

	public GameObject ToggleOn0;

    void Start()
    {
        
    }
	public override void Interact()
	{
		ToggleOff0.SetActive(false);
		ToggleOff1.SetActive(false);
		ToggleOn0.SetActive(true);
	
        Networking.LocalPlayer.TeleportTo(Destination.transform.position, 
                                          Destination.transform.rotation, 
                                          VRC_SceneDescriptor.SpawnOrientation.Default, 
                                          false);
										  
										  
	}
}
