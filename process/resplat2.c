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
	int16_t x, y, z;
	uint8_t sx, sy, sz;
	int8_t rx, ry, rz; // rq is shadowed.
	uint8_t cr, cg, cb;
	int8_t reserved0;
	
	int nOriginalID; // Not included in general payload to textures.
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

typedef struct
{
	int index;
	float sort;
} splatorder;

int comparSplatOrder(const void* p1, const void* p2)
{
	splatorder * a1 = (splatorder*)p1;
	splatorder * a2 = (splatorder*)p2;
	if( a1->sort < a2->sort ) return -1;
	else if( a1->sort > a2->sort ) return 1;
	else return 0;
}

int main( int argc, char ** argv )
{
	int i;
	
	// Clear out index array with sentinel values.
	int * offsetListAsInt = &offsetList.pos[0];
	for( i = 0; i < sizeof( offsetList ) / sizeof( offsetList.pos[0] ); i++ )
		offsetListAsInt[i] = -1;

	if( argc != 6 )
	{
		fprintf( stderr, "Error: Usage: resplat [.ply file] [out, quaded .ply file] [out, .asset image file] [out, .asset mesh file] [out, .asset image cardinal sort file]\n" );
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

		// from  https://github.com/antimatter15/splat/blob/main/main.js
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
		if( scale2s[1] < scale2s[2] )
		{
			// around X
			float quatRotYZ[4] = {0.70710678, 0.70710678, 0.0, 0.0 };
			mathRotateVectorByQuaternion( scale2s, quatRotYZ, scale2s );
			mathQuatApply(realrot, realrot, quatRotYZ);
			scale2s[0] = absf( scale2s[0] );
			scale2s[1] = absf( scale2s[1] );
			scale2s[2] = absf( scale2s[2] );
		}

		// Make sure Z is the smallest scale.
		if( scale2s[0] < scale2s[2] )
		{
			// around Y
			float quatRotXZ[4] = {0.70710678, 0.0, 0.70710678, 0.0 };
			mathRotateVectorByQuaternion( scale2s, quatRotXZ, scale2s );
			mathQuatApply(realrot, realrot, quatRotXZ);
			scale2s[0] = absf( scale2s[0] );
			scale2s[1] = absf( scale2s[1] );
			scale2s[2] = absf( scale2s[2] );
		}
		
		// Make sure Y is smaller than X
		if( scale2s[0] < scale2s[1] )
		{
			// around Z
			float quatRotXY[4] = {0.70710678, 0.0, 0.0, 0.70710678 };
			mathRotateVectorByQuaternion( scale2s, quatRotXY, scale2s );
			mathQuatApply(realrot, realrot, quatRotXY);
			scale2s[0] = absf( scale2s[0] );
			scale2s[1] = absf( scale2s[1] );
			scale2s[2] = absf( scale2s[2] );
		}

		int isx = ( log(scale2s[0])/log(2.71828) + 7.0 ) * 32.0 + 0.5;
		int isy = ( log(scale2s[1])/log(2.71828) + 7.0 ) * 32.0 + 0.5;
		int isz = ( log(scale2s[2])/log(2.71828) + 7.0 ) * 32.0 + 0.5;
		if( isx > 255 ) isx = 255;
		if( isy > 255 ) isy = 255;
		if( isz > 255 ) isz = 255;
		if( isx < 0 ) isx = 0;
		if( isy < 0 ) isx = 0;
		if( isz < 0 ) isz = 0;
		so->sx = isx;
		so->sy = isy;
		so->sz = isz;
		
		//printf( "%3d %3d %3d %f %f %f from %f %f %f\n", isx, isy, isz, scale2s[0], scale2s[1], scale2s[2], realscale[0], realscale[1], realscale[2] );
		// Is this the right way??? XXX TODO CHECK ME.
		if( realrot[0] < 0 )
		{
			realrot[1] *= -1;
			realrot[2] *= -1;
			realrot[3] *= -1;
		}

		int rrx = realrot[1] * 127 - 0.5;
		int rry = realrot[2] * 127 - 0.5;
		int rrz = realrot[3] * 127 - 0.5;
		so->rx = rrx;
		so->ry = rry;
		so->rz = rrz;

		int cr = si->color[0] * 255.0;		if( cr > 255 ) cr = 255;
		int cg = si->color[1] * 255.0;		if( cg > 255 ) cg = 255;
		int cb = si->color[2] * 255.0;		if( cb > 255 ) cb = 255;
		so->cr = cr;
		so->cg = cg;
		so->cb = cb;
		
		so->nOriginalID = v;
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
		free( pData );
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

	
	// Generate a cardinal sorting map.  The idea is we sort from each direction (left/right/up/down/forward/backwards) and
	// then the geometry shader will figure out what the best one to use is.
	{
		int w = 2048;
		int h = (( splatOutCount + w - 1 ) / w) * 6;
		int dir = 0;
		splatorder * sortedSplats = malloc( sizeof( splatorder ) * splatOutCount );
		uint32_t * pData = malloc( w * h * 4 );
		for( dir = 0; dir < 6; dir++ )
		{
			int splatno = 0;

			for( splatno = 0; splatno < splatOutCount; splatno++ )
			{
				splatOut * so = &splatsOut[splatno];
				sortedSplats[splatno].index = splatno;
				if( dir == 0 )
					sortedSplats[splatno].sort = so->x;
				else if( dir == 1 )
					sortedSplats[splatno].sort = -so->x;
				else if( dir == 2 )
					sortedSplats[splatno].sort = so->y;
				else if( dir == 3 )
					sortedSplats[splatno].sort = -so->y;
				else if( dir == 4 )
					sortedSplats[splatno].sort = so->z;
				else if( dir == 5 )
					sortedSplats[splatno].sort = -so->z;
			}

			qsort (sortedSplats, splatOutCount, sizeof(splatorder), comparSplatOrder);
  
			splatno = 0;
			int x, y;
			for( y = 0; y < h/6; y++ )
			for( x = 0; x < w; x++ )
			{
				if( splatno >= splatOutCount ) continue;
				int order = sortedSplats[splatno++].index;
				pData[x+y*w + (dir*(h/6)*w)] = order;
			}			
		}
		WriteUnityImageAsset( argv[5], pData, w * h * 4, w, h, 1, UTE_R_FLOAT );
		free( pData );
	}

	fprintf( fOut, "ply\n" );
	//fprintf( fOut, "format binary_little_endian 1.0\n" );
	fprintf( fOut, "format ascii 1.0\n" );
	fprintf( fOut, "element vertex %d\n", splatOutCount * 4 );
	fprintf( fOut, "property float x\n" );
	fprintf( fOut, "property float y\n" );
	fprintf( fOut, "property float z\n" );
	fprintf( fOut, "property float red\n" );
	fprintf( fOut, "property float green\n" );
	fprintf( fOut, "property float blue\n" );
	fprintf( fOut, "property float alpha\n" );
	fprintf( fOut, "property float s\n" );
	fprintf( fOut, "property float t\n" );
	fprintf( fOut, "property float u\n" );
	fprintf( fOut, "element face %d\n", splatOutCount );
	fprintf( fOut, "property list uint uint vertex_indices\n" );
	fprintf( fOut, "end_header\n" );

	for( v = 0; v < splatOutCount; v++ )
	{
		splatOut * so = &splatsOut[v];

		float pos[3] = { 
			so->x / 65535.0f * inversescale + centers[0],
			so->y / 65535.0f * inversescale + centers[1],
			so->z / 65535.0f * inversescale + centers[2] };
		float scale[3] = {
			exp(((so->sx - 0.5) / 32.0) - 7.0),
			exp(((so->sy - 0.5) / 32.0) - 7.0),
			exp(((so->sz - 0.5) / 32.0) - 7.0) };
		float rot[4] = { 0, so->rx / 127.0, so->ry / 127.0, so->rz / 127.0 };  //XXX This looks wrong, shouldn't it contain 0.5?
		float rsum = ( rot[1]*rot[1] + rot[2]*rot[2] + rot[3]*rot[3] );
		if( rsum < 1.0 )
			rot[0] = sqrtf( 1.0 - rsum );
		float cr = so->cr / 255.0;
		float cg = so->cg / 255.0;
		float cb = so->cb / 255.0;

		float v1[7] = { 1, 1, 0 };
		float v2[7] = { -1, 1, 0 };
		float v3[7] = { 1, -1, 0 };
		float v4[7] = { -1, -1, 0 };
		
		int orig = so->nOriginalID;

		//if( rsum > 1.0 )
		//	printf( "%f %f %f  %f %f %f  %f %f %f %f / %f / %d %d %d\n", pos[0], pos[1], pos[2], scale[0], scale[1], scale[2], rot[0], rot[1], rot[2], rot[3], rsum, so->rx, so->ry, so->rz );
		
		mathVectorScale( v1, scale, v1 );
		mathRotateVectorByQuaternion( v1, rot, v1 );
		mathVectorAdd( v1, pos, v1 );
		fprintf( fOut, "%f %f %f %f %f %f %f %f %f %d\n", v1[0], v1[1], v1[2], cr, cg, cb, 1.0, 1.0, 1.0, orig );
		//fwrite( v1, sizeof( v1 ), 1, fOut );
		//fprintf( fOut, "v %f %f %f %f %f %f\n", v1[0], v1[1], v1[2], si->color[0], si->color[1], si->color[2] );

		mathVectorScale( v2, scale, v2 );
		mathRotateVectorByQuaternion( v2, rot, v2 );
		mathVectorAdd( v2, pos, v2 );
		fprintf( fOut, "%f %f %f %f %f %f %f %f %f %d\n", v2[0], v2[1], v2[2], cr, cg, cb, 1.0, 0.0, 1.0, orig );
		//fwrite( v2, sizeof( v2 ), 1, fOut );
		//fprintf( fOut, "v %f %f %f %f %f %f\n", v2[0], v2[1], v2[2], si->color[0], si->color[1], si->color[2] );

		mathVectorScale( v3, scale, v3 );
		mathRotateVectorByQuaternion( v3, rot, v3 );
		mathVectorAdd( v3, pos, v3 );
		fprintf( fOut, "%f %f %f %f %f %f %f %f %f %d\n", v3[0], v3[1], v3[2], cr, cg, cb, 1.0, 1.0, 0.0, orig );
		//fwrite( v3, sizeof( v3 ), 1, fOut );
		//fprintf( fOut, "v %f %f %f %f %f %f\n", v3[0], v3[1], v3[2], si->color[0], si->color[1], si->color[2] );

		mathVectorScale( v4, scale, v4 );
		mathRotateVectorByQuaternion( v4, rot, v4 );
		mathVectorAdd( v4, pos, v4 );
		fprintf( fOut, "%f %f %f %f %f %f %f %f %f %d\n", v4[0], v4[1], v4[2], cr, cg, cb, 1.0, 0.0, 0.0, orig );
		//fwrite( v4, sizeof( v4 ), 1, fOut );
		//fprintf( fOut, "v %f %f %f %f %f %f\n", v4[0], v4[1], v4[2], si->color[0], si->color[1], si->color[2] );
	}

	for( v = 0; v < splatOutCount; v++ )
	{
		int vb = v*4;
		//fprintf( fOut, "f %d %d %d\nf %d %d %d\n", vb+1, vb+2, vb+3, vb+4, vb+5, vb+6 );
		uint32_t voo[5] = { 4, vb+0, vb+2, vb+3, vb+1 };
		fprintf( fOut, "%d %d %d %d %d\n", voo[0], voo[1], voo[2], voo[3], voo[4] );
	}
	
	fclose( fOut );

	return 0;	
}
