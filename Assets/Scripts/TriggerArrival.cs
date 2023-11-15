using UdonSharp;
using UnityEngine;
using VRC.SDKBase;
using VRC.Udon;

public class TriggerArrival : UdonSharpBehaviour
{
	public GameObject[] ToggleOff;
	public GameObject[] ToggleOn;

    void Start()
    {
        
    }
	public override void OnPlayerTriggerEnter(VRCPlayerApi player)
	{
		if( player.isLocal )
		{
			foreach( GameObject o in ToggleOff )
			{
				o.SetActive(false);
			}
			foreach( GameObject o in ToggleOn )
			{
				o.SetActive(true);
			}
		}
	}
}
