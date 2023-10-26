//!!! THIS IS NOT A SCRIPT YOU SHOULD RUN
// It just exists to create a .asset file of the right type so I can look at it and learn how to make another mesh.

#if UNITY_EDITOR
using UnityEngine;
using UnityEditor;

public class MakeSlapSplatGeometry : MonoBehaviour
{
	[MenuItem("Tools/Make_SlapSplatMesh")]
	static void CreateMesh_()
	{
		Mesh mesh = new Mesh();
		
		int trueverts = 1;
		int trueinds  = 131073;
		int[] indices = new int[trueinds];
		Vector3[] vertices = new Vector3[trueverts];
		int j = 0;
		for( j = 0; j < trueinds; j++ )
		{
			indices[j] = 0;
		}
		
		mesh.vertices = vertices;
		mesh.bounds = new Bounds(new Vector3(0,28,15), new Vector3(40*2, 40*2, 60*2));
		mesh.SetIndices(indices, MeshTopology.Points, 0, false, 0);
		AssetDatabase.CreateAsset(mesh, "Assets/slapsplat/TestMesh.asset");
	}
}
#endif