
using UdonSharp;
using UnityEngine;
using VRC.SDKBase;
using VRC.Udon;

public class WarpTo : UdonSharpBehaviour
{
	public GameObject Destination;	
	public GameObject[] ToggleOff;
	public GameObject[] ToggleOn;

    void Start()
    {
        
    }
	public override void Interact()
	{
		foreach( GameObject o in ToggleOff )
		{
			o.SetActive(false);
		}
		foreach( GameObject o in ToggleOn )
		{
			o.SetActive(true);
		}
	
        Networking.LocalPlayer.TeleportTo(Destination.transform.position, 
                                          Destination.transform.rotation, 
                                          VRC_SceneDescriptor.SpawnOrientation.Default, 
                                          false);
										  
										  
	}
}
