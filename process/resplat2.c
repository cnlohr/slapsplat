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


struct splatOut_t
{
	/*
		x = 16 bits
		y = 16 bits
		z = 16 bits
		sx = 8 bits
		sy = 8 bits

		/// MASKED w = 8 bits ///
		x = 8 bits
		y = 8 bits
		z = 8 bits
		
		r = 8 bits
		g = 8 bits
		b = 8 bits
		/// MASKED a = 8 bits ///
	*/

	int16_t x, y, z;
	int8_t sx, sy, sz;
	int8_t rx, ry, rz; // rq is shadowed.
	int8_t cr, cg, cb;
	int8_t reserved0;
} * splatsOut;

typedef struct splatIn_t splatIn;
typedef struct splatOut_t splatOut;


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

void mathQuatApply(float* qout, const float* q1, const float* q2)
{
    // NOTE: Does not normalize - you will need to normalize eventually.
    float tmpw, tmpx, tmpy;
    tmpw    = (q1[0] * q2[0]) - (q1[1] * q2[1]) - (q1[2] * q2[2]) - (q1[3] * q2[3]);
    tmpx    = (q1[0] * q2[1]) + (q1[1] * q2[0]) + (q1[2] * q2[3]) - (q1[3] * q2[2]);
    tmpy    = (q1[0] * q2[2]) - (q1[1] * q2[3]) + (q1[2] * q2[0]) + (q1[3] * q2[1]);
    qout[3] = (q1[0] * q2[3]) + (q1[1] * q2[2]) - (q1[2] * q2[1]) + (q1[3] * q2[0]);
    qout[2] = tmpy;
    qout[1] = tmpx;
    qout[0] = tmpw;
}

float absf( float f )
{
	if( f < 0 )
		return -f;
	return f;
}

float sqrtf( float f )
{
	return sqrt( f );
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
	int splatOutCount = 0;
	
	float mins[3] = { 1e20, 1e20, 1e20 };
	float maxs[3] = {-1e20,-1e20,-1e20 };

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
		
		if( si->pos[0] > maxs[0] ) maxs[0] = si->pos[0];
		if( si->pos[1] > maxs[1] ) maxs[1] = si->pos[1];
		if( si->pos[2] > maxs[2] ) maxs[2] = si->pos[2];

		if( si->pos[0] < mins[0] ) mins[0] = si->pos[0];
		if( si->pos[1] < mins[1] ) mins[1] = si->pos[1];
		if( si->pos[2] < mins[2] ) mins[2] = si->pos[2];

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
	}
	
	// Compute max bounding box.
	float inversescale = 0;
	if( maxs[0] - mins[0] > inversescale ) inversescale = maxs[0] - mins[0];
	if( maxs[1] - mins[1] > inversescale ) inversescale = maxs[1] - mins[1];
	if( maxs[2] - mins[2] > inversescale ) inversescale = maxs[2] - mins[2];
	
	float centers[3] = { (maxs[0] + mins[0])/2.0, (maxs[1] + mins[1])/2.0, (maxs[2] + mins[2])/2.0 };

	for( v = 0; v < splatsInCount; v++ )
	{
		splatIn * si = &splatsIn[v];

		if( si->opacity < 0.3 ) continue;
		if( si->color[0] < 0.0 ) continue;
		if( si->color[1] < 0.0 ) continue;
		if( si->color[2] < 0.0 ) continue;

		float realrot[4] = { si->rot[3], si->rot[0], si->rot[1], si->rot[2] }; // Why??
		float rrscale = 1./sqrtf( realrot[0] * realrot[0] + realrot[1] * realrot[1] + realrot[2] * realrot[2] + realrot[3] * realrot[3] );
		realrot[0] *= rrscale;
		realrot[1] *= rrscale;
		realrot[2] *= rrscale;
		realrot[3] *= rrscale;

		float realscale[3] = { exp( si->scale[0] ) * 2.5, exp( si->scale[1] ) * 2.5, exp( si->scale[2] ) * 2.5 };
		realscale[0] = absf( realscale[0] );
		realscale[1] = absf( realscale[1] );
		realscale[2] = absf( realscale[2] );

		splatsOut = realloc( splatsOut, (splatOutCount + 1) * sizeof( splatOut ) );
		splatOut * so = &splatsOut[splatOutCount++];
		
		float ox = (si->pos[0] - centers[0]) / inversescale * 65535;
		float oy = (si->pos[1] - centers[1]) / inversescale * 65535;
		float oz = (si->pos[2] - centers[2]) / inversescale * 65535;
		so->x = ox;
		so->y = oy;
		so->z = oz;

		float scale2s[3] = { realscale[0], realscale[1], realscale[2] };
		
		// Make sure Z is the smallest scale.
		if( realscale[0] < realscale[2] )
		{
			// around Y
			float quatRotXZ[4] = {0.70710678, 0.0, 0.70710678, 0.0 };
			mathRotateVectorByQuaternion( scale2s, quatRotXZ, scale2s );
			mathQuatApply(realrot, realrot, quatRotXZ);
		}

		if( realscale[1] < realscale[2] )
		{
			// around X
			float quatRotYZ[4] = {0.70710678, 0.70710678, 0.0, 0.0 };
			mathRotateVectorByQuaternion( scale2s, quatRotYZ, scale2s );
			mathQuatApply(realrot, realrot, quatRotYZ);
		}
		
		if( realscale[0] < realscale[1] )
		{
			// around Z
			float quatRotXY[4] = {0.70710678, 0.0, 0.0, 0.70710678 };
			mathRotateVectorByQuaternion( scale2s, quatRotXY, scale2s );
			mathQuatApply(realrot, realrot, quatRotXY);
		}

		scale2s[0] = absf( scale2s[0] );
		scale2s[1] = absf( scale2s[1] );
		scale2s[2] = absf( scale2s[2] );

		so->sx = ( log(scale2s[0])/log(2.71828) + 7.0 ) * 32.0 + 0.5;
		so->sy = ( log(scale2s[1])/log(2.71828) + 7.0 ) * 32.0 + 0.5;
		so->sz = ( log(scale2s[2])/log(2.71828) + 7.0 ) * 32.0 + 0.5;
		// Is this the right way??? XXX TODO CHECK ME.
		so->rx = realrot[0] * 127 - 0.5;
		so->ry = realrot[1] * 127 - 0.5;
		so->rz = realrot[2] * 127 - 0.5;

		int cr = si->color[0] * 255.0;		if( cr > 255 ) cr = 255;
		int cg = si->color[1] * 255.0;		if( cg > 255 ) cg = 255;
		int cb = si->color[2] * 255.0;		if( cb > 255 ) cb = 255;
		so->cr = cr;
		so->cg = cg;
		so->cb = cb;
	}

	printf( "Loaded %d splats\n", splatsInCount );
	printf( "Culled to %d splats\n", splatOutCount );
	
	{
		int w = 1024;
		int h = ( splatOutCount + w - 1 ) / w;
		int d = 1;
		int size = w * h * sizeof( float ) * d * 4;
		uint8_t * pData = calloc( size, 1 );
		int x, y;
		int p = 0;
		printf( "Output asset dimensions: %d x %d x %d\n", w, h, d );
		for( y = 0; y < h; y++ )
		for( x = 0; x < w; x++ )
		{
			if( p < splatOutCount)
			{
				splatOut * so = &splatsOut[p];
				uint8_t * bO = (uint8_t*)&pData[4*((x+y*w)*sizeof( float ))*d];
				bO[0] = so->x & 0xff;
				bO[1] = so->x >> 8;
				bO[2] = so->y & 0xff;
				bO[3] = so->y >> 8;
				bO[4] = so->z & 0xff;
				bO[5] = so->z >> 8;
				bO[6] = so->sx;
				bO[7] = so->sy;
				bO[8] = so->sz;
				bO[9] = so->rx;
				bO[10] = so->ry;
				bO[11] = so->rz;
				bO[12] = so->cr;
				bO[13] = so->cg;
				bO[14] = so->cb;
				bO[15] = so->reserved0;
			}
			p++;
		}
		WriteUnityImageAsset( argv[3], pData, size, w, h, 1, UTE_RGBA_FLOAT );
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
		"  m_IndexBuffer: ", splatOutCount, 1 );
			// Repeat 0000 # of indices
	for( v = 0; v < splatOutCount; v++ )
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
}
