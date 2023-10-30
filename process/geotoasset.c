#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "unitytexturewriter.h"
#include "common.h"

struct
{
	int pos[3];
	int color[4];
} offsetList;

int verticesInCount = 0;
int facesInCount = 0;

struct vertexIn_t
{
	float pos[3];
	float color[4];
} * verticesIn;

typedef struct vertexIn_t vertexIn;

int main( int argc, char ** argv )
{
	int i;
	
	// Clear out index array with sentinel values.
	int * offsetListAsInt = &offsetList.pos[0];
	for( i = 0; i < sizeof( offsetList ) / sizeof( offsetList.pos[0] ); i++ )
		offsetListAsInt[i] = -1;

	if( argc != 5 )
	{
		fprintf( stderr, "Error: Usage: resplat [geo .ply file] [out, .asset image file] [out, .asset mesh file] [out, .asset image cardinal sort file]\n" );
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
					if( strcmp( param2, "vertex" ) == 0 )
					{
						verticesInCount = atoi( param3 );
					}
					else if( strcmp( param2, "face" ) == 0 )
					{
						facesInCount = atoi( param3 );
					}
					else
					{
						fprintf( stderr, "Error: invalid PLY contents - only vertices + faces allowed for splats. Got %s\n", param2 );
						return -7;
					}
				}
				else if( strcmp( param1, "property" ) == 0 )
				{
					if( strcmp( param2, "float" ) == 0 )
					{
							 if( strcmp( param3, "x" ) == 0 ) offsetList.pos[0] = parameterNumber;
						else if( strcmp( param3, "y" ) == 0 ) offsetList.pos[1] = parameterNumber;
						else if( strcmp( param3, "z" ) == 0 ) offsetList.pos[2] = parameterNumber;
						parameterNumber+=4;
					}
					else if( strcmp( param2, "uchar" ) == 0 )
					{
							 if( strcmp( param3, "red" ) == 0 ) offsetList.color[0] = parameterNumber;
						else if( strcmp( param3, "green" ) == 0 ) offsetList.color[1] = parameterNumber;
						else if( strcmp( param3, "blue" ) == 0 ) offsetList.color[2] = parameterNumber;
						else if( strcmp( param3, "alpha" ) == 0 ) offsetList.color[3] = parameterNumber;
						parameterNumber+=1;
					}
					else if( strcmp( param2, "list" ) == 0 )
					{
						// List for faces.
					}
					else
					{
						fprintf( stderr, "Error: invalid PLY contents - unknown properties (%s) for splats.\n", param2 );
						return -7;
					}
				}				
				else if( strcmp( param1, "comment" ) == 0 )
				{
					// Do nothing.
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
			else if( strcmp( param1, "comment" ) == 0 )
			{
				// Do nothing.
			}
			else
			{
				fprintf( stderr, "Error: Unknown parameter: \"%s\"\n", param1 );
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
	
	if( facesInCount <= 0 )
	{
		fprintf( stderr, "Error: face list in not found\n" );
		return -12;
	}
	
	if( verticesInCount <= 0 )
	{
		fprintf( stderr, "Error: vertex list in not found\n" );
		return -12;
	}
	
	int v;
	printf( "Generating vertices list (%d) / faces (%d)\n", verticesInCount, facesInCount );
	verticesIn = malloc( sizeof( vertexIn ) * verticesInCount );
	splatsIn = malloc( sizeof( splatIn ) * facesInCount );
	
	float mins[3] = { 1e20, 1e20, 1e20 };
	float maxs[3] = {-1e20,-1e20,-1e20 };

	for( v = 0; v < verticesInCount; v++ )
	{
		uint8_t buffer[parameterNumber];
		if( fread( buffer, sizeof( buffer ), 1, f ) != 1 )
		{
			fprintf( stderr, "Error: premature EOF found\n" );
			return -13;
		}
		
		vertexIn * vi = &verticesIn[v];
		vi->pos[0] = *(float*)&buffer[ offsetList.pos[0] ];
		vi->pos[1] = *(float*)&buffer[ offsetList.pos[1] ];
		vi->pos[2] = *(float*)&buffer[ offsetList.pos[2] ];
		
		vi->color[0] = *(uint8_t*)&buffer[ offsetList.color[0] ] / 255.0;
		vi->color[1] = *(uint8_t*)&buffer[ offsetList.color[1] ] / 255.0;
		vi->color[2] = *(uint8_t*)&buffer[ offsetList.color[2] ] / 255.0;
		vi->color[3] = *(uint8_t*)&buffer[ offsetList.color[3] ] / 255.0;
	}

	printf( "Loaded vertices\n" );
	int fid;
	for( fid = 0; fid < facesInCount; fid++ )
	{
		int nFaces = getc( f );
		uint32_t faces[nFaces];
		if( fread( faces, nFaces * sizeof( uint32_t ), 1, f ) != 1 )
		{
			fprintf( stderr, "Error: premature EOF found (faces)\n" );
			return -15;
		}
		if( nFaces != 4 )
		{
			fprintf( stderr, "Warning: Found non-quad.  Only quads supported.\n" );
			continue;
		}
		
		splatIn * si = &splatsIn[splatsInCount++];

		vertexIn * vs[4];
		int i;
		float avgx = 0, avgy = 0, avgz = 0;
		for( i = 0; i < 4; i++ )
		{
			vs[i] = &verticesIn[faces[i]];
			avgx += vs[i]->pos[0];
			avgy += vs[i]->pos[1];
			avgz += vs[i]->pos[2];
		}
		avgx /= 4; avgy /= 4; avgz /= 4;
		
		si->pos[0] = avgx; si->pos[1] = avgy; si->pos[2] = avgz;
		si->color[0] = vs[0]->color[0];
		si->color[1] = vs[0]->color[1];
		si->color[2] = vs[0]->color[2];
		si->opacity  = vs[0]->color[3];
		
		
		if( si->pos[0] > maxs[0] ) maxs[0] = si->pos[0];
		if( si->pos[1] > maxs[1] ) maxs[1] = si->pos[1];
		if( si->pos[2] > maxs[2] ) maxs[2] = si->pos[2];
		if( si->pos[0] < mins[0] ) mins[0] = si->pos[0];
		if( si->pos[1] < mins[1] ) mins[1] = si->pos[1];
		if( si->pos[2] < mins[2] ) mins[2] = si->pos[2];

		
		// XXX TODO FIXME.
		int axis1edge = 1;
		int axis2edge = 2;
		
		float edge1[3];
		float edge2[3];
		mathVectorSub( edge1, vs[0]->pos, vs[axis1edge]->pos );
		mathVectorSub( edge2, vs[0]->pos, vs[axis2edge]->pos );
		
		si->scale[0] = mathLength( edge1 );
		si->scale[1] = mathLength( edge2 );
		si->scale[2] = 0;

		// Generate Quaternion from each axis.
		float up[3];
		mathCrossProduct( up, edge1, edge2 );
		mathVectorScalar( up, up, 1.0/mathLength( up ) );

		// What does it take to rotate 0,0,1 to this?  That's our first quaternion.
		float fromup[3] = { 0, 0, 1 };
		float qfirst[4];
		mathCreateQuatFromTwoVectors( qfirst, fromup, up );

		si->rot[0] = qfirst[0];
		si->rot[1] = qfirst[1];
		si->rot[2] = qfirst[2];
		si->rot[3] = qfirst[3];
	}
	
	printf( "Got %d splats\n", splatsInCount );
	
	CommonOutput( argv[2], argv[3], argv[4], mins, maxs );

	
	return 0;
}