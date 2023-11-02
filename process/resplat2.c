#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "unitytexturewriter.h"
#include "common.h"

struct
{
	int pos[3];
	int scale[3];
	int color[3];
	int opacity;
	int rot[4];
} offsetList;

int main( int argc, char ** argv )
{
	int i;
	
	// Clear out index array with sentinel values.
	int * offsetListAsInt = &offsetList.pos[0];
	for( i = 0; i < sizeof( offsetList ) / sizeof( offsetList.pos[0] ); i++ )
		offsetListAsInt[i] = -1;

	if( argc != 6 && argc != 3 )
	{
		fprintf( stderr, "Error: Usage: resplat2 [.ply file] [out, quaded .ply file] [ [out, .asset image file] [out, .asset mesh file] [out, .asset image cardinal sort file] ]\n" );
		fprintf( stderr, "The out assets are optional. But if one is specified all must be specified.  Otherwise it only takes in a polycam .ply, and outputs a .ply mesh with vertex colors.\n" );
		return -5;
	}

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
	printf( "Generating splatsin (%d)\n", splatsInCount );
	splatsIn = malloc( sizeof( splatIn ) * splatsInCount );
	
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

		si->scale[0] = exp( buffer[ offsetList.scale[0] ] ) * 2.5;
		si->scale[1] = exp( buffer[ offsetList.scale[1] ] ) * 2.5;
		si->scale[2] = exp( buffer[ offsetList.scale[2] ] ) * 2.5;
		
		const float SH_C0 = 0.28209479177387814;
		si->color[0] = 0.5 + SH_C0 * buffer[ offsetList.color[0] ];
		si->color[1] = 0.5 + SH_C0 * buffer[ offsetList.color[1] ];
		si->color[2] = 0.5 + SH_C0 * buffer[ offsetList.color[2] ];
		si->opacity = (1 / (1 + exp(-buffer[ offsetList.opacity ])));

		si->rot[0] = buffer[ offsetList.rot[0] ];
		si->rot[1] = buffer[ offsetList.rot[1] ];
		si->rot[2] = buffer[ offsetList.rot[2] ];
		si->rot[3] = buffer[ offsetList.rot[3] ];
	}
	
	if( argc >= 6 )
		CommonOutput( argv[3], argv[4], argv[5], mins, maxs );

	printf( "Outputting %s\n", argv[2] );
	FILE * fOut = fopen( argv[2], "w" );
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

/*
		// This works and can be used to validate that our computation of the compressed splats is good.

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
*/

		int orig = so->nOriginalID;
		float pos[3] = { so->orx, so->ory, so->orz };
		float rot[4] = { so->orrq, so->orrx, so->orry, so->orrz };
		float scale[3] = { so->orsx, so->orsy, so->orsz };
		float cr = so->orr, cg = so->org, cb = so->orb;

		float v1[7] = { 1, 1, 0 };
		float v2[7] = { -1, 1, 0 };
		float v3[7] = { 1, -1, 0 };
		float v4[7] = { -1, -1, 0 };

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
