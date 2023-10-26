#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "unitytexturewriter.h"

struct
{
	int pos[3];
	int scale[3];
	int color[3];
	int opacity;
	int rot[4];
} offsetList;

int splatsInCount = 0;

struct splatIn_t
{
	float pos[3];
	float scale[3];
	float color[3];
	float opacity;
	float rot[4];
} * splatsIn;

typedef struct splatIn_t splatIn;


void mathCrossProduct(float* p, const float* a, const float* b)
{
    float tx = a[1] * b[2] - a[2] * b[1];
    float ty = a[2] * b[0] - a[0] * b[2];
    p[2]     = a[0] * b[1] - a[1] * b[0];
    p[1]     = ty;
    p[0]     = tx;
}

void mathRotateVectorByQuaternion(float* pout, const float* q, const float* p)
{
    // return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
    float iqo[3];
    mathCrossProduct(iqo, q + 1 /*.xyz*/, p);
    iqo[0] += q[0] * p[0];
    iqo[1] += q[0] * p[1];
    iqo[2] += q[0] * p[2];
    float ret[3];
    mathCrossProduct(ret, q + 1 /*.xyz*/, iqo);
    pout[0] = ret[0] * 2.0 + p[0];
    pout[1] = ret[1] * 2.0 + p[1];
    pout[2] = ret[2] * 2.0 + p[2];
}

void mathRotateVectorByInverseOfQuaternion(float* pout, const float* q, const float* p)
{
    // General note: Performing a transform this way can be about 20-30% slower than a well formed 3x3 matrix.
    // return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
    float iqo[3];
    mathCrossProduct(iqo, p, q + 1 /*.xyz*/);
    iqo[0] += q[0] * p[0];
    iqo[1] += q[0] * p[1];
    iqo[2] += q[0] * p[2];
    float ret[3];
    mathCrossProduct(ret, iqo, q + 1 /*.xyz*/);
    pout[0] = ret[0] * 2.0 + p[0];
    pout[1] = ret[1] * 2.0 + p[1];
    pout[2] = ret[2] * 2.0 + p[2];
}

void mathVectorScale( float * pOut, const float * scale, const float * p )
{
	pOut[0] = p[0] * scale[0];
	pOut[1] = p[1] * scale[1];
	pOut[2] = p[2] * scale[2];
}

void mathVectorAdd( float * pOut, const float * offset, const float * p )
{
	pOut[0] = p[0] + offset[0];
	pOut[1] = p[1] + offset[1];
	pOut[2] = p[2] + offset[2];
}

int main( int argc, char ** argv )
{
	int i;
	
	// Clear out index array with sentinel values.
	int * offsetListAsInt = &offsetList.pos[0];
	for( i = 0; i < sizeof( offsetList ) / sizeof( offsetList.pos[0] ); i++ )
		offsetListAsInt[i] = -1;

	if( argc != 5 )
	{
		fprintf( stderr, "Error: Usage: resplat [.ply file] [out, .ply file] [out, .asset image file] [out, .asset mesh file]\n" );
		fprintf( stderr, "WARNING: PLY file output is currently broken.\n" );
		return -5;
	}

	FILE * fOut = fopen( argv[2], "w" );
	FILE * f = fopen( argv[1], "rb" );
	if( !f )
	{
		fprintf( stderr, "Error: can't open splat file \"%s\"\n", argv[1] );
		return -6;
	}
	char thisline[1024];
	int tlp = 0;
	int parameterNumber = 0;
	while( !feof( f ) && !ferror( f ) )
	{
		int c = fgetc( f );
		if( c == '\n' )
		{
			thisline[tlp] = 0;
			tlp = 0;
			char param1[1024];
			char param2[1024];
			char param3[1024];
			int pars = sscanf( thisline, "%s %s %s", param1, param2, param3 );
			if( pars == 3 )
			{
				// All the interesting pars are 3 parameters.
				if( strcmp( param1, "format" ) == 0 )
				{
					if( strcmp( param2, "binary_little_endian" ) != 0 || atof( param3 ) < 0.99999 )
					{
						fprintf( stderr, "Error: invalid PLY format.\n" );
						return -7;
					}
				}
				else if( strcmp( param1, "element" ) == 0 )
				{
					if( strcmp( param2, "vertex" ) != 0 )
					{
						fprintf( stderr, "Error: invalid PLY contents - only vertices allowed for splats.\n" );
						return -7;
					}
					splatsInCount = atoi( param3 );
				}
				else if( strcmp( param1, "property" ) == 0 )
				{
					if( strcmp( param2, "float" ) != 0 )
					{
						fprintf( stderr, "Error: invalid PLY contents - only float properties allowed for splats.\n" );
						return -7;
					}
					     if( strcmp( param3, "x" ) == 0 ) offsetList.pos[0] = parameterNumber;
					else if( strcmp( param3, "y" ) == 0 ) offsetList.pos[1] = parameterNumber;
					else if( strcmp( param3, "z" ) == 0 ) offsetList.pos[2] = parameterNumber;
					else if( strcmp( param3, "scale_0" ) == 0 ) offsetList.scale[0] = parameterNumber;
					else if( strcmp( param3, "scale_1" ) == 0 ) offsetList.scale[1] = parameterNumber;
					else if( strcmp( param3, "scale_2" ) == 0 ) offsetList.scale[2] = parameterNumber;
					else if( strcmp( param3, "f_dc_0" ) == 0 ) offsetList.color[0] = parameterNumber;
					else if( strcmp( param3, "f_dc_1" ) == 0 ) offsetList.color[1] = parameterNumber;
					else if( strcmp( param3, "f_dc_2" ) == 0 ) offsetList.color[2] = parameterNumber;
					else if( strcmp( param3, "opacity" ) == 0 ) offsetList.opacity = parameterNumber;
					else if( strcmp( param3, "rot_0" ) == 0 ) offsetList.rot[0] = parameterNumber;
					else if( strcmp( param3, "rot_1" ) == 0 ) offsetList.rot[1] = parameterNumber;
					else if( strcmp( param3, "rot_2" ) == 0 ) offsetList.rot[2] = parameterNumber;
					else if( strcmp( param3, "rot_3" ) == 0 ) offsetList.rot[3] = parameterNumber;
					parameterNumber++;
				}
				else
				{
					fprintf( stderr, "Error: Unknown parameter: %s\n", param1 );
					return -9;
				}
			}
			else if( strcmp( param1, "end_header" ) == 0 )
			{
				printf( "Found %d parameters\n", parameterNumber );
				break;
			}
			else if( strcmp( param1, "ply" ) == 0 )
			{
				// Do nothing.
			}
			else
			{
				fprintf( stderr, "Error: Unknown parameter: %s\n", param1 );
				return -9;
			}
		}
		else
		{
			if( c >= 32 )
				thisline[tlp++] = c;
		}
	}
	if( feof( f ) || ferror( f ) )
	{
		fprintf( stderr, "Error: malformed file\n" );
		return -10;
	}

	for( i = 0; i < sizeof( offsetList ) / sizeof( offsetList.pos[0] ); i++ )
	{
		if( offsetListAsInt[i] < 0 )
		{
			fprintf( stderr, "Error: Not all fields are found.  Missing field %d\n", i );
			return -11;
		}
	}
	
	if( splatsInCount <= 0 )
	{
		fprintf( stderr, "Error: splat list in not found\n" );
		return -12;
	}
	
	int v;

	splatsIn = malloc( sizeof( splatIn ) * splatsInCount );
	
	for( v = 0; v < splatsInCount; v++ )
	{
		float buffer[parameterNumber];
		if( fread( buffer, sizeof( buffer ), 1, f ) != 1 )
		{
			fprintf( stderr, "Error: premature EOF found\n" );
			return -13;
		}
		
		splatIn * si = &splatsIn[v];
		si->pos[0] = buffer[ offsetList.pos[0] ];
		si->pos[1] = buffer[ offsetList.pos[1] ];
		si->pos[2] = buffer[ offsetList.pos[2] ];

		si->scale[0] = buffer[ offsetList.scale[0] ];
		si->scale[1] = buffer[ offsetList.scale[1] ];
		si->scale[2] = buffer[ offsetList.scale[2] ];


		const float SH_C0 = 0.28209479177387814;
		si->color[0] = 0.5 + SH_C0 * buffer[ offsetList.color[0] ];
		si->color[1] = 0.5 + SH_C0 * buffer[ offsetList.color[1] ];
		si->color[2] = 0.5 + SH_C0 * buffer[ offsetList.color[2] ];

		si->opacity = (1 / (1 + exp(-buffer[ offsetList.opacity ])));

		si->rot[3] = buffer[ offsetList.rot[0] ];
		si->rot[0] = buffer[ offsetList.rot[1] ];
		si->rot[1] = buffer[ offsetList.rot[2] ];
		si->rot[2] = buffer[ offsetList.rot[3] ];
/*
		
				const qlen = Math.sqrt(
					attrs.rot_0 ** 2 +
						attrs.rot_1 ** 2 +
						attrs.rot_2 ** 2 +
						attrs.rot_3 ** 2,
				);

				rot[0] = (attrs.rot_0 / qlen) * 128 + 128;
				rot[1] = (attrs.rot_1 / qlen) * 128 + 128;
				rot[2] = (attrs.rot_2 / qlen) * 128 + 128;
				rot[3] = (attrs.rot_3 / qlen) * 128 + 128;*/
				
/*		printf( "%6.3f %6.3f %6.3f  %6.3f %6.3f %6.3f   %6.3f %6.3f %6.3f   %6.3f   %6.3f %6.3f %6.3f %6.3f\n",
			si->pos[0], si->pos[1], si->pos[2],
			si->scale[0], si->scale[1], si->scale[2],
			si->color[0], si->color[1], si->color[2],
			si->opacity,
			si->rot[0], si->rot[1], si->rot[2], si->rot[3] );
			*/
	}

	printf( "Loaded %d splats\n", splatsInCount );
	
	fprintf( fOut, "ply\n" );
	//fprintf( fOut, "format binary_little_endian 1.0\n" );
	fprintf( fOut, "format ascii 1.0\n" );
	fprintf( fOut, "element vertex %d\n", splatsInCount * 4 );
	fprintf( fOut, "property float x\n" );
	fprintf( fOut, "property float y\n" );
	fprintf( fOut, "property float z\n" );
	fprintf( fOut, "property float red\n" );
	fprintf( fOut, "property float green\n" );
	fprintf( fOut, "property float blue\n" );
	fprintf( fOut, "property float alpha\n" );
	fprintf( fOut, "element face %d\n", splatsInCount * 2 );
	fprintf( fOut, "property list uint uint vertex_indices\n" );
	fprintf( fOut, "end_header\n" );

	{
		int w = 1024;
		int h = ( splatsInCount + w - 1 ) / w;
		int d = 4;
		int size = w * h * sizeof( float ) * d * 4;
		uint8_t * pData = calloc( size, 1 );
		int x, y;
		int p = 0;
		printf( "Output asset dimensions: %d x %d x %d\n", w, h, d );
		for( y = 0; y < h; y++ )
		for( x = 0; x < w; x++ )
		{
			if( p < splatsInCount )
			{
				splatIn * si = &splatsIn[p];
				float * fO = (float*)&pData[4*((x+y*w)*sizeof( float ))*d];
				fO[0] = si->pos[0];
				fO[1] = si->pos[1];
				fO[2] = si->pos[2];
				fO[3] = 0.0;
				fO[4] = si->scale[0];
				fO[5] = si->scale[1];
				fO[6] = si->scale[2];
				fO[7] = 0.0;
				fO[8] = si->color[0];
				fO[9] = si->color[1];
				fO[10] = si->color[2];
				fO[11] = si->opacity;
				fO[12] = si->rot[0];
				fO[13] = si->rot[1];
				fO[14] = si->rot[2];
				fO[15] = si->rot[3];
			}
			p++;
		}
		WriteUnityImageAsset( argv[3], pData, size, d, w, h, UTE_RGBA_FLOAT | UTE_FLAG_IS_3D );
	}
	
	FILE * fFM = fopen( argv[4], "w" );
	fprintf( fFM,
		"%%YAML 1.1\n"
		"%%TAG !u! tag:unity3d.com,2011:\n"
		"--- !u!43 &4300000\n"
		"Mesh:\n"
		"  m_ObjectHideFlags: 0\n"
		"  m_CorrespondingSourceObject: {fileID: 0}\n"
		"  m_PrefabInstance: {fileID: 0}\n"
		"  m_PrefabAsset: {fileID: 0}\n"
		"  m_Name: SlapSplatMesh\n"
		"  serializedVersion: 10\n"
		"  m_SubMeshes:\n"
		"  - serializedVersion: 2\n"
		"    indexCount: %d\n"
		"    topology: 5\n"
		"    vertexCount: %d\n"
		"    localAABB:\n"
		"      m_Center: {x: 0, y: 0, z: 0}\n"
		"      m_Extent: {x: 0, y: 0, z: 0}\n"
		"  m_IsReadable: 1\n"
		"  m_KeepVertices: 1\n"
		"  m_KeepIndices: 1\n"
		"  m_IndexFormat: 0\n"
		"  m_IndexBuffer: ", splatsInCount, 1 );
			// Repeat 0000 # of indices
	for( v = 0; v < splatsInCount; v++ )
	{
		fprintf( fFM, "0000" );
	}
	fprintf( fFM, "\n" );
	fprintf( fFM,
		"  m_VertexData:\n"
		"    serializedVersion: 3\n"
		"    m_VertexCount: %d\n"
		"    m_Channels:\n"
		"    - stream: 0\n"
		"      offset: 0\n"
		"      format: 0\n"
		"      dimension: 3\n"
		"    m_DataSize: 600\n"
		"    _typelessdata: 000000000000000000000000\n"
		"  m_LocalAABB:\n"
		"    m_Center: {x: 0, y: 28, z: 15}\n"
		"    m_Extent: {x: 40, y: 40, z: 60}\n"
		"  m_MeshOptimizationFlags: 1\n", 1 );
	fclose( fFM );


	return 0;


	for( v = 0; v < splatsInCount; v++ )
	{
		splatIn * si = &splatsIn[v];

		float v1[7] = { 1, 1, 0, si->color[0], si->color[1], si->color[2] };
		float v2[7] = { -1, 1, 0, si->color[0], si->color[1], si->color[2] };
		float v3[7] = { 1, -1, 0, si->color[0], si->color[1], si->color[2] };
		float v4[7] = { -1, -1, 0, si->color[0], si->color[1], si->color[2] };
		uint8_t colors[4] = { si->color[0] * 255.5, si->color[1] * 255.5, si->color[2] * 255.5, si->opacity * 255.5 };
		mathVectorScale( v1, si->scale, v1 );
		mathRotateVectorByInverseOfQuaternion( v1, si->rot, v1 );
		mathVectorAdd( v1, si->pos, v1 );
		fprintf( fOut, "%f %f %f %f %f %f %f\n", v1[0], v1[1], v1[2], si->color[0], si->color[1], si->color[2], si->opacity );
		//fwrite( v1, sizeof( v1 ), 1, fOut );
		//fprintf( fOut, "v %f %f %f %f %f %f\n", v1[0], v1[1], v1[2], si->color[0], si->color[1], si->color[2] );

		mathVectorScale( v2, si->scale, v2 );
		mathRotateVectorByInverseOfQuaternion( v2, si->rot, v2 );
		mathVectorAdd( v2, si->pos, v2 );
		fprintf( fOut, "%f %f %f %f %f %f %f\n", v2[0], v2[1], v2[2], si->color[0], si->color[1], si->color[2], si->opacity );
		//fwrite( v2, sizeof( v2 ), 1, fOut );
		//fprintf( fOut, "v %f %f %f %f %f %f\n", v2[0], v2[1], v2[2], si->color[0], si->color[1], si->color[2] );

		mathVectorScale( v3, si->scale, v3 );
		mathRotateVectorByInverseOfQuaternion( v3, si->rot, v3 );
		mathVectorAdd( v3, si->pos, v3 );
		fprintf( fOut, "%f %f %f %f %f %f %f\n", v3[0], v3[1], v3[2], si->color[0], si->color[1], si->color[2], si->opacity );
		//fwrite( v3, sizeof( v3 ), 1, fOut );
		//fprintf( fOut, "v %f %f %f %f %f %f\n", v3[0], v3[1], v3[2], si->color[0], si->color[1], si->color[2] );

		mathVectorScale( v4, si->scale, v4 );
		mathRotateVectorByInverseOfQuaternion( v4, si->rot, v4 );
		mathVectorAdd( v4, si->pos, v4 );
		fprintf( fOut, "%f %f %f %f %f %f %f\n", v4[0], v4[1], v4[2], si->color[0], si->color[1], si->color[2], si->opacity );
		//fwrite( v4, sizeof( v4 ), 1, fOut );
		//fprintf( fOut, "v %f %f %f %f %f %f\n", v4[0], v4[1], v4[2], si->color[0], si->color[1], si->color[2] );		

	}

	for( v = 0; v < splatsInCount; v++ )
	{
		int vb = v*4;
		//fprintf( fOut, "f %d %d %d\nf %d %d %d\n", vb+1, vb+2, vb+3, vb+4, vb+5, vb+6 );
		uint32_t voo[8] = { 3, vb+0, vb+1, vb+2, 3, vb+1, vb+2, vb+3 };
		//fwrite( voo, sizeof( voo ), 1, fOut );
		fprintf( fOut, "%d %d %d %d\n%d %d %d %d\n", voo[0], voo[1], voo[2], voo[3], voo[4], voo[5], voo[6], voo[7] );
	}
	
	fclose( fOut );

	
}
