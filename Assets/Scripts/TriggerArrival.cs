using UdonSharp;
using UnityEngine;
using VRC.SDKBase;
using VRC.Udon;

public class TriggerArrival : UdonSharpBehaviour
{
	public GameObject ToggleOff0;
	public GameObject ToggleOff1;
	public GameObject ToggleOn0;

    void Start()
    {
        
    }
	public override void OnPlayerTriggerEnter(VRCPlayerApi player)
	{
		if( player.isLocal )
		{
			ToggleOff0.SetActive(false);
			ToggleOff1.SetActive(false);
			ToggleOn0.SetActive(true);
		}
	}
}
