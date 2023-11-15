#ifndef _COMMON_H
#define _COMMON_H

#include <math.h>

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

void mathVectorScalar( float * pOut, const float * p, const float scalar )
{
	pOut[0] = p[0] * scalar;
	pOut[1] = p[1] * scalar;
	pOut[2] = p[2] * scalar;
}

void mathVectorAdd( float * pOut, const float * offset, const float * p )
{
	pOut[0] = p[0] + offset[0];
	pOut[1] = p[1] + offset[1];
	pOut[2] = p[2] + offset[2];
}

void mathVectorSub( float * pOut, const float * a, const float * b )
{
	pOut[0] = a[0] - b[0];
	pOut[1] = a[1] - b[1];
	pOut[2] = a[2] - b[2];
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

float mathLength( const float * v1 )
{
	return sqrtf( v1[0]*v1[0] + v1[1]*v1[1] + v1[2]*v1[2] );
}

float mathDotProduct( const float *f1, const float * f2 )
{
	return f1[0] * f2[0] + f1[1] * f2[1] + f1[2] * f2[2];
}

void mathVectorNormalize( float * fout, const float * fin )
{
	float imag = fin[0] * fin[0] + fin[1] * fin[1] + fin[2] * fin[2];
	mathVectorScalar( fout, fin, 1.0/sqrtf(imag) );
}

// WARNING: Vectors MUST be normalized.  XXX TODO: VALIDATE FROM/TO.
void mathCreateQuatFromTwoVectors( float * q, const float * from, const float * to )
{
	// Make sure vectors aren't pointing opposite directions.
	float dot = mathDotProduct( from, to );
	if( dot < -0.99999 )
	{
		q[0] = 0;
		q[1] = 0;
		q[2] = 1;
		q[3] = 0;
		return;
	}
	
	float half[3];
	mathVectorAdd( half, from, to );
	mathVectorNormalize( half, half );
	q[0] = mathDotProduct( from, half );
	mathCrossProduct( q+1, from, half );
}

void mathCreateQuatFromBasis( float * q, const float * m33 )
{
	#define m00 m33[0]
	#define m01 m33[3]
	#define m02 m33[6]
	#define m10 m33[1]
	#define m11 m33[4]
	#define m12 m33[7]
	#define m20 m33[2]
	#define m21 m33[5]
	#define m22 m33[8]
	#define qw q[0]
	#define qx q[1]
	#define qy q[2]
	#define qz q[3]

	float tr = m00 + m11 + m22;
	
	if (tr > 0) { 
		float S = sqrt(tr+1.0) * 2; // S=4*qw 
		qw = 0.25 * S;
		qx = (m21 - m12) / S;
		qy = (m02 - m20) / S; 
		qz = (m10 - m01) / S; 
	} else if ((m00 > m11)&(m00 > m22)) { 
		float S = sqrt(1.0 + m00 - m11 - m22) * 2; // S=4*qx 
		qw = (m21 - m12) / S;
		qx = 0.25 * S;
		qy = (m01 + m10) / S; 
		qz = (m02 + m20) / S; 
	} else if (m11 > m22) { 
		float S = sqrt(1.0 + m11 - m00 - m22) * 2; // S=4*qy
		qw = (m02 - m20) / S;
		qx = (m01 + m10) / S; 
		qy = 0.25 * S;
		qz = (m12 + m21) / S; 
	} else { 
		float S = sqrt(1.0 + m22 - m00 - m11) * 2; // S=4*qz
		qw = (m10 - m01) / S;
		qx = (m02 + m20) / S;
		qy = (m12 + m21) / S;
		qz = 0.25 * S;
	}
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



int splatsInCount = 0;
int splatOutCount = 0;

struct splatIn_t
{
	float pos[3];
	float scale[3];
	float color[3];
	float opacity;
	float rot[4];
	
	float colAndSH[48];
} * splatsIn;

struct splatOut_t
{
	int16_t x, y, z;
	uint8_t sx, sy, sz;
	int8_t rx, ry, rz; // rq is shadowed.
	uint8_t cr, cg, cb;
	int8_t reserved0;
	
	int nOriginalID; // Not included in general payload to textures.
	float orx, ory, orz;
	float orsx, orsy, orsz;
	float orrq, orrx, orry, orrz;
	float orr, org, orb;


	float colAndSH[48];
} * splatsOut;

typedef struct splatIn_t splatIn;
typedef struct splatOut_t splatOut;

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

void CommonOutput( const char * ImageAsset, const char * MeshAsset, const char * OrderAsset, const char * SHAsset, float * mins, float * maxs )
{
	int v;
	
	// Compute max bounding box.
	float inversescale = 0;
	if( maxs[0] - mins[0] > inversescale ) inversescale = maxs[0] - mins[0];
	if( maxs[1] - mins[1] > inversescale ) inversescale = maxs[1] - mins[1];
	if( maxs[2] - mins[2] > inversescale ) inversescale = maxs[2] - mins[2];
	
	float centers[3] = { (maxs[0] + mins[0])/2.0, (maxs[1] + mins[1])/2.0, (maxs[2] + mins[2])/2.0 };

	printf( "Bounding Box [%f %f %f] - [%f %f %f]\n", mins[0], mins[1], mins[2], maxs[0], maxs[1], maxs[2] );

	splatsOut = malloc( splatsInCount * sizeof( splatOut ) );

	printf( "Genearting Splats Out List\n" );

	for( v = 0; v < splatsInCount; v++ )
	{
		splatIn * si = &splatsIn[v];

		if( si->opacity < 0.3 ) continue;
		if( si->color[0] < 0.0 ) si->color[0] = 0.0;
		if( si->color[1] < 0.0 ) si->color[1] = 0.0;
		if( si->color[2] < 0.0 ) si->color[2] = 0.0;

		// from  https://github.com/antimatter15/splat/blob/main/main.js
		float realrot[4] = { si->rot[0], si->rot[1], si->rot[2], si->rot[3] }; // Why??
		float rrscale = 1./sqrtf( realrot[0] * realrot[0] + realrot[1] * realrot[1] + realrot[2] * realrot[2] + realrot[3] * realrot[3] );
		realrot[0] *= rrscale;
		realrot[1] *= rrscale;
		realrot[2] *= rrscale;
		realrot[3] *= rrscale;

		float realscale[3] = { si->scale[0], si->scale[1], si->scale[2] };
		realscale[0] = absf( realscale[0] );
		realscale[1] = absf( realscale[1] );
		realscale[2] = absf( realscale[2] );

		splatOut * so = &splatsOut[splatOutCount++];
		
		float initposx = si->pos[0] - centers[0];
		float initposy = si->pos[1] - centers[1];
		float initposz = si->pos[2] - centers[2];
		float ox = (initposx) / inversescale * 65535;
		float oy = (initposy) / inversescale * 65535;
		float oz = (initposz) / inversescale * 65535;
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

		if( scale2s[0] < 0 ) scale2s[0] = -scale2s[0];
		if( scale2s[1] < 0 ) scale2s[1] = -scale2s[1];
		if( scale2s[2] < 0 ) scale2s[2] = -scale2s[2];
		if( scale2s[0] < 0.000001 ) scale2s[0] = 0.000001;
		if( scale2s[1] < 0.000001 ) scale2s[1] = 0.000001;
		if( scale2s[2] < 0.000001 ) scale2s[2] = 0.000001;
		
//		if( scale2s[0] > 1 || scale2s[1] > 1 || scale2s[2] > 1 )
//			printf( "%f %f %f -> %f\n", scale2s[0], scale2s[1], scale2s[2], ln2_simple( scale2s[2] ) );
		int isx = ( log(scale2s[0])/log(2.71828) + 7.0 ) * 32.0 + 0.5;
		int isy = ( log(scale2s[1])/log(2.71828) + 7.0 ) * 32.0 + 0.5;
		int isz = ( log(scale2s[2])/log(2.71828) + 7.0 ) * 32.0 + 0.5;
		//printf( "%3d %3d %3d << %f %f %f\n", isx, isy, isz, scale2s[0], scale2s[1], scale2s[2] );
		if( isx > 255 ) isx = 255;
		if( isy > 255 ) isy = 255;
		if( isz > 255 ) isz = 255;
		if( isx < 0 ) isx = 0;
		if( isy < 0 ) isy = 0;
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
		
		// Save off high fidelity values.
		so->orx = initposx; so->ory = initposy; so->orz = initposz;
		so->orrq = realrot[0]; so->orrx = realrot[1]; so->orry = realrot[2]; so->orrz = realrot[3];
		so->orsx = scale2s[0]; so->orsy = scale2s[1]; so->orsz = scale2s[2];
		so->orr = si->color[0]; so->org = si->color[1]; so->orb = si->color[2];


		int cr = si->color[0] * 255.0;		if( cr > 255 ) cr = 255;
		int cg = si->color[1] * 255.0;		if( cg > 255 ) cg = 255;
		int cb = si->color[2] * 255.0;		if( cb > 255 ) cb = 255;
		so->cr = cr;
		so->cg = cg;
		so->cb = cb;
		
		so->nOriginalID = v;
		
		int j;
		for( j = 0; j < 48; j++ )
			so->colAndSH[j] = si->colAndSH[j];
	}

	printf( "Loaded %d splats\n", splatsInCount );
	printf( "Culled to %d splats\n", splatOutCount );
	
	if( ImageAsset )
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
		WriteUnityImageAsset( ImageAsset, pData, size, w, h, 1, UTE_RGBA_FLOAT );
		free( pData );
	}
	
	if( MeshAsset )
	{
		FILE * fFM = fopen( MeshAsset, "w" );
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
	}
	
	// Generate a cardinal sorting map.  The idea is we sort from each direction (left/right/up/down/forward/backwards) and
	// then the geometry shader will figure out what the best one to use is.
	if( OrderAsset )
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
		WriteUnityImageAsset( OrderAsset, pData, w * h * 4, w, h, 1, UTE_R_FLOAT );
		free( pData );
	}
	
	if( SHAsset )
	{
		// colAndSH is 48 floats.
		int w = 4096;
		int h = 4 * ( splatOutCount + (w/4) - 1 ) / (w/4);
		int size = w * h * sizeof( uint8_t ) * 3;
		uint8_t * pData = calloc( size, 1 );
		int x, y;
		int p = 0;
		printf( "Output SH dimensions: %d x %d x UTE_RGB24\n", w, h );
		float minsh = 1e20;
		float maxsh = -1e20;

		//FILE * fDebug = fopen( "C:\\temp\\debug.txt", "w" );

		for( y = 0; y < h; y+=4 )
		{
			for( x = 0; x < w; x+=4 )
			{
				if( p < splatOutCount)
				{
					splatOut * so = &splatsOut[p];
					uint8_t * bO[4] =
					{
						&pData[3*((x+(y+0)*w))],
						&pData[3*((x+(y+1)*w))],
						&pData[3*((x+(y+2)*w))],
						&pData[3*((x+(y+3)*w))]
					};
					int j;
					for( j = 0; j < 16; j++ )
					{
						int x;
						for( x = 0; x < 3; x++ )
						{
							float s = so->colAndSH[j*3+x];

							float sabs = (s<0)?-s:s;
							s = sqrtf( sabs ) * ((s<0)?-1:1);

							if( j != 0 )
							{
								if( s > maxsh ) maxsh = s;
								if( s < minsh ) minsh = s;
							}
							int v = (j == 0) ? ( (s + 2.0) * 64) : ( (s + 0.5) * 255.5);
							if( v < 0 ) v = 0;
							if( v > 255 ) v = 255;
							bO[j/4][(j%4)*3+x] = v;
							//fprintf( fDebug, "%f,", s );
						}
					}
				}
				//fprintf( fDebug, "\n" );
				p++;
			}
			//printf( "%d\n", y );
		}
		printf( "SH Range: %f %f\n", minsh, maxsh );
		WriteUnityImageAsset( SHAsset, pData, size, w, h, 1, UTE_RGB24 );// UTE_RGB24  | ( 1 << UTE_FLAG_IS_CHANGE_COLOR_SPACE_V ) );
		free( pData );
		printf( "Done\n" );
	}
}

#endif


