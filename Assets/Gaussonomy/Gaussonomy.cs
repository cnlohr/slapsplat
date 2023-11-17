
using UdonSharp;
using UnityEngine;
using VRC.SDKBase;
using VRC.Udon;
using UnityEngine.Rendering;

public class Gaussonomy : UdonSharpBehaviour
{
	public Camera Stage1;
	public Camera Stage2;
	public bool enableRender = true;
	public Material Stage1Debug;
	public Material Stage2Mat;
	public Material Stage2Debug;
	public Material Stage3;
	private RenderTexture [] RTs1 = new RenderTexture[8];
	private RenderBuffer [] RBs1 = new RenderBuffer[8];
	private RenderTexture [] RTs2 = new RenderTexture[8];
	private RenderBuffer [] RBs2 = new RenderBuffer[8];
    void Start()
    {
		RenderTextureDescriptor rtd = new RenderTextureDescriptor();
		rtd.autoGenerateMips = false;
		rtd.bindMS = false;
		rtd.colorFormat = RenderTextureFormat.ARGBFloat;
		rtd.depthBufferBits = 24;
		rtd.dimension = TextureDimension.Tex2D;
		rtd.enableRandomWrite = true; // Should be true for a later one?
		rtd.height = 256;
		rtd.width = 256;
		rtd.sRGB = false;
		rtd.msaaSamples = 1;
		rtd.volumeDepth = 1;
		rtd.autoGenerateMips = false;

		int i;
		for( i = 0; i < 8; i++ )
		{
			rtd.depthBufferBits = (i == 0 ) ? 24 : 0; // Subsequent RTs should be no depth buffer.

			RenderTexture rt = RTs1[i] = new RenderTexture( rtd );
			RBs1[i] = rt.colorBuffer;
			string pName = "_Stage_" + i;
			Stage1Debug.SetTexture( pName, rt );
			Stage2Mat.SetTexture( pName, rt );
			
			rt = RTs2[i] = new RenderTexture( rtd );
			RBs2[i] = rt.colorBuffer;
			Stage2Debug.SetTexture( pName, rt );
			Stage3.SetTexture( pName, rt );
		}
		Stage1.SetTargetBuffers( RBs1, RTs1[0].depthBuffer );
		Stage2.SetTargetBuffers( RBs2, RTs2[0].depthBuffer );
		Stage1.enabled = false;
		Stage2.enabled = false;
    }
	void Update()
	{
		if( enableRender )
		{
			Stage1.Render();
			Stage2.Render();
			//Graphics.SetRenderTarget( RBs2, RTs2[0].depthBuffer );
			//Graphics.Blit( null, null, Stage2, -1); 
		}
	}
}
