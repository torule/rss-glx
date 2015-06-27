/*
 * Copyright (C) 2002, 2009  Sören Sonnenburg <sonne@debian.org>
 *
 * FX done Breakpoint 2002, code recovered, converted and polished into
 * xscreensaver hack on Apr 12 at Breakpoint 2009.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Lorenz attractor screensaver 
 */

const char *hack_name = "lorenz";

#include "driver.h"
#include "rsRand.h"
#include <GL/gl.h>
#include <GL/glu.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <rsMath/rsVec.h>
#include <rsMath/rsQuat.h>


#define num_satellites_default 25
#define num_points_default 100000
#define line_width_attractor_default 2
#define line_width_satellites_default 16
#define camera_speed_default 0.3
#define linear_cutoff_default 0.2
#define camera_angle_default 45

static float elapsedTime;

static int line_width_satellites=line_width_satellites_default;
static int line_width_attractor=line_width_attractor_default;
static float camera_speed=camera_speed_default;
static float linear_cutoff=linear_cutoff_default;

static int num_points_max=-1;
static int num_precomputed_points=num_points_default;
static int num_points;
static int num_satellites=num_satellites_default;
static float lorenz_a=11;
static float lorenz_b=15;
static float lorenz_c=3;
static float lorenz_dt=0.02;

static float camera_angle=camera_angle_default;
static float camera_angle_anim[2] = {5, 179};
static float camera_angle_anim_speed=0.1;
static float mean[3] = { 0, 0, 0 };
static float simTime;
static float lastn0 = 0;
static float flipn0 = 1;
static float deltaflipn0 = 0;

static int width=800;
static int height=600;

static GLfloat col_ambient[] =  { 0.0, 0.1, 0.4, 0 };

static GLfloat l0_diffuse[] = 	 { 1.0, 1.0, 0.0, 1.0 };
static GLfloat l0_specular[] =	 { 0.1, 0.1, 0.05, 0.0 };
static GLfloat l0_att[] =	 { 0.5 };
static GLfloat l0_qatt[] =	 { 5.01 };
static GLfloat l0_cutoff[] =	 { 100 };

static float* lorenz_coords;
static float* lorenz_path;
static float* satellite_times;
static float* satellite_speeds;

static inline float distance(float P0x, float P0y, float P0z,
		float P1x, float P1y, float P1z)
{
	float x=P0x-P1x;
	float y=P0y-P1y;
	float z=P0z-P1z;
	return sqrt(x*x+y*y+z*z);
}

static inline void norm_one(float* x, float* y, float* z)
{
	float n=sqrt(((*x)*(*x))+((*y)*(*y))+((*z)*(*z)));
	(*x)/=n;
	(*y)/=n;
	(*z)/=n;
}

inline void calc_normal(float P0x, float P0y, float P0z,
		float P1x, float P1y, float P1z,
		float P2x, float P2y, float P2z,
		float* Nx, float* Ny, float* Nz)
{
	(*Nx)= (P1y - P0y) * (P2z - P0z) - (P1z - P0z) * (P2y - P0y);
	(*Ny)= (P1z - P0z) * (P2x - P0x) - (P1x - P0x) * (P2z - P0z);
	(*Nz)= (P1x - P0x) * (P2y - P0y) - (P1y - P0y) * (P2x - P0x);
}

inline float angle(float P0x, float P0y, float P0z,
		float P1x, float P1y, float P1z,
		float P2x, float P2y, float P2z)
{
	return atan2(distance(P0x,P0y,P0z, P1x, P1y, P1z), distance(P0x,P0y,P0z, P2x, P2y, P2z));
}


inline void coords_at_time(float* from, float t, float* x, float* y, float* z)
{
	int u=(int) t;
	float s;
	s=(t-floor(t));
	*x=(1-s)*from[3*u]+s*from[3*(u+1)];
	*y=(1-s)*from[3*u+1]+s*from[3*(u+1)+1];
	*z=(1-s)*from[3*u+2]+s*from[3*(u+1)+2];
}

inline void normal_at_time(float* from, float t, float* x, float* y, float* z)
{
	float p1[3];
	float p2[3];
	float p3[3];
	coords_at_time(from, t, &p1[0], &p1[1], &p1[2]);
	coords_at_time(from, t-1, &p2[0], &p2[1], &p2[2]);
	coords_at_time(from, t+1, &p3[0], &p3[1], &p3[2]);
	calc_normal(p1[0], p1[1], p1[2],
			p2[0], p2[1], p2[2],
			p3[0], p3[1], p3[2],
			x,y,z);
}

inline float distance_to_line(float P0x, float P0y, float P0z,
		float P1x, float P1y, float P1z,
		float P2x, float P2y, float P2z)
{

	return distance(P0x,P0y,P0z, P1x,P1y,P1z)*sin(angle(P0x,P0y,P0z, P1x,P1y,P1z, P2x,P2y,P2z));
}

void reduce_points(int cutoff)
{
	int j=0;
	int start=0;
	int end=0;
	float dist=1;
	int current_offs=0;

	num_points=num_precomputed_points;

	while (current_offs<num_points-1 && start<num_points-1 && end<num_points-1)
	{
		dist=0;
		for (end=start; end<num_points && dist<linear_cutoff; end++)
		{
			for (j=start; j<=end && dist<linear_cutoff; j++)
			{
				dist=distance_to_line(lorenz_coords[3*start],
						lorenz_coords[3*start+1],lorenz_coords[3*start+2],
						lorenz_coords[3*j],lorenz_coords[3*j+1],lorenz_coords[3*j+2],
						lorenz_coords[3*end],lorenz_coords[3*end+1],lorenz_coords[3*end+2]);
			}
		}

		end--;
		current_offs++;
		lorenz_path[3*current_offs]=lorenz_coords[3*end];
		lorenz_path[3*current_offs+1]=lorenz_coords[3*end+1];
		lorenz_path[3*current_offs+2]=lorenz_coords[3*end+2];
		start=end;
	}
#ifdef DEBUG
	printf("reduced from %d to %d\n", num_points, current_offs);
#endif
	num_points=current_offs;

	if (num_points_max>0 && num_points>num_points_max)
		num_points=num_points_max;
}

void init_line_strip(void) 
{
	int i;
	float n[3];

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glShadeModel(GL_FLAT);
	glEnable(GL_LIGHTING);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	glEnable(GL_BLEND);
	glEnable(GL_LIGHT0);

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, col_ambient);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);

	glNewList(1, GL_COMPILE);
	glLineWidth(line_width_attractor);
	glBegin(GL_LINE_STRIP);
	for (i=100; i<num_points-20; i++)
	{
		glVertex3fv(&lorenz_path[i*3]);

		calc_normal(lorenz_path[3*i], lorenz_path[3*i+1], lorenz_path[3*i+2], 
				lorenz_path[3*(i+3)], lorenz_path[3*(i+3)+1], lorenz_path[3*(i+3)+2],
				lorenz_path[3*(i+5)], lorenz_path[3*(i+5)+2], lorenz_path[3*(i+5)+2],
				&n[0],&n[1],&n[2]);

		norm_one(&n[0], &n[1], &n[2]);

		glNormal3fv(n);
	}
	glEnd();
	glEndList();
}


void display(void)
{
	int satellites=0;
	float n[3];
	float p1[3];
	float p2[3];
	float p3[3];
	float l=0;

	//set cam
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();          

	coords_at_time(lorenz_coords, simTime,   &p1[0], &p1[1], &p1[2]);
	coords_at_time(lorenz_coords, simTime+10,&p2[0], &p2[1], &p2[2]);
	coords_at_time(lorenz_coords, simTime+15,&p3[0], &p3[1], &p3[2]);

	calc_normal(p1[0], p1[1], p1[2],
			p2[0], p2[1], p2[2],
			p3[0], p3[1], p3[2],
			&n[0], &n[1], &n[2]);

	norm_one(&n[0], &n[1], &n[2]);

	// n[0]'s sign changes when transitioning between the lobes...
	if ((lastn0 >= 0) != (n[0] >= 0)) {
		// if we've completed a transition, one in four chance whether to transition again.
		if ((deltaflipn0 >= 1.0) && (rsRandi(4) == 0)) {
			flipn0 = flipn0 * -1;
			deltaflipn0 = 0;
		}
	}

	lastn0 = n[0];

	deltaflipn0 += elapsedTime;

	// during transition, slerp between flipped states
	if (deltaflipn0 < 1.0) {
		float la[3];

		la[0] = p2[0] - p1[0];
		la[1] = p2[1] - p1[1];
		la[2] = p2[2] - p1[2];

		norm_one(&la[0], &la[1], &la[2]);

		rsQuat normal, flipped;
		normal.make(0.0, la[0], la[1], la[2]);
		flipped.make(M_PI, la[0], la[1], la[2]);
		rsQuat f;

		if (flipn0 == -1)
			f.slerp(normal, flipped, deltaflipn0);
		else
			f.slerp(flipped, normal, deltaflipn0);

		rsVec rn = f.apply(rsVec(n[0], n[1], n[2]));
		n[0] = rn[0];
		n[1] = rn[1];
		n[2] = rn[2];
	} else {
		n[0] = n[0] * flipn0;
		n[1] = n[1] * flipn0;
		n[2] = n[2] * flipn0;
	}


	gluLookAt(p1[0]+0.9*n[0], p1[1]+0.9*n[1], p1[2]+0.9*n[2],
			p2[0], p2[1], p2[2],
			n[0],n[1],n[2]);

	coords_at_time(lorenz_coords, simTime+10,&p2[0], &p2[1], &p2[2]);
	coords_at_time(lorenz_coords, simTime+2, &p3[0], &p3[1], &p3[2]);

	//set light	
	GLfloat light_position0[] = { p2[0]+n[0], p2[1]+n[1], p2[2]+n[2], 1.0 };
	GLfloat light_dir0[] = { n[0], n[1], n[2], 1.0 };

	glPushMatrix();

	glLightfv(GL_LIGHT0, GL_DIFFUSE, l0_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, l0_specular);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position0);
	glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, light_dir0);
	glLightfv(GL_LIGHT0, GL_CONSTANT_ATTENUATION, l0_att);
	glLightfv(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, l0_qatt);
	glLightfv(GL_LIGHT0, GL_SPOT_CUTOFF, l0_cutoff);
	glPopMatrix();

	glClear (GL_COLOR_BUFFER_BIT);
	glCallList(1);

	glDisable(GL_LIGHTING);
	for (satellites=0; satellites<num_satellites; satellites++)
	{
		glPushMatrix();
		float x,y,z;
		coords_at_time(lorenz_coords, satellite_times[satellites], &x,&y,&z);

		glLineWidth(line_width_satellites);
		glBegin(GL_LINE_STRIP);
		float s=37*elapsedTime*satellite_speeds[satellites]*num_points/num_precomputed_points;

		float maxl= ( 10*(fabs(s)) < 3 ) ? 3 : 10*(fabs(s));
		float stepl=(fabs(s)/maxl);
		for (l=0; l<maxl; l+=stepl)
		{
			coords_at_time(lorenz_coords, satellite_times[satellites]+l, &x,&y,&z);
			glVertex3d(x,y,z);
			glNormal3d(light_dir0[0], light_dir0[1], light_dir0[2]);
			if (s<0)
				glColor4f(0.4,0.3,1.0/(l+1),0.9/(l+1));
			else 
				glColor4f(0.4,0.3,1.0/(maxl-l+1),0.9/(maxl-l+1));

		}
		glEnd();
		glPopMatrix();

		satellite_times[satellites]+=s;
		if (satellite_times[satellites]>num_points-20)
			satellite_times[satellites]-=num_points-20;
		if (satellite_times[satellites]<0)
			satellite_times[satellites]+=num_points-20;
	}
	glEnable(GL_BLEND);
	glEnable(GL_LIGHTING);

	glFlush ();
}

void set_camera()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(camera_angle, ((GLfloat) width)/height,0.0,1000);
}

void reshape (int w, int h)
{
	width=w;
	height=h;
#ifdef DEBUG
	printf("width=%d, height=%d\n", width, height);
#endif

	set_camera();
	glMatrixMode (GL_MODELVIEW);
	glViewport (0, 0, (GLsizei) w, (GLsizei) h); 
}

void init_satellites()
{
	int i;

	if (num_satellites>0)
	{
		satellite_times = (float *)malloc(sizeof(float)*num_satellites);
		satellite_speeds = (float *)malloc(sizeof(float)*num_satellites);

		for (i=0; i<num_satellites; i++)
		{
			satellite_times[i]=rsRandf(num_points);
			satellite_speeds[i]=10*camera_speed*(rsRandf(1.0) - 0.5);
		}
	}
}

void precompute_lorenz_array()
{
	int i;
	float max[3] = { 1,1,1 };

	lorenz_coords=(float*) malloc(3*num_precomputed_points*sizeof(float));
	lorenz_path=(float*) malloc(3*num_precomputed_points*sizeof(float));

	lorenz_coords[0]=20;
	lorenz_coords[1]=5;
	lorenz_coords[2]=-5;

	for (i=0; i<num_precomputed_points-1; i++)
	{
		lorenz_coords[(i+1)*3] = lorenz_coords[i*3] + lorenz_a * 
			( lorenz_coords[i*3+1] - lorenz_coords[i*3] ) * lorenz_dt;

		lorenz_coords[(i+1)*3+1] = lorenz_coords[i*3+1] +
			( lorenz_b * lorenz_coords[i*3] - lorenz_coords[i*3+1] -
			  lorenz_coords[i*3] * lorenz_coords[i*3+2] ) * lorenz_dt;

		lorenz_coords[(i+1)*3+2] = lorenz_coords[i*3+2] +
			( lorenz_coords[i*3] * lorenz_coords[i*3+1] -
			  lorenz_c * lorenz_coords[i*3+2] ) * lorenz_dt;
	}

	for (i=0; i<num_precomputed_points; i++)
	{
		mean[0]+=lorenz_coords[i*3];
		mean[1]+=lorenz_coords[i*3+1];
		mean[2]+=lorenz_coords[i*3+2];
	}

	mean[0]/=num_precomputed_points;
	mean[1]/=num_precomputed_points;
	mean[2]/=num_precomputed_points;

	for (i=0; i<num_precomputed_points; i++)
	{
		lorenz_coords[i*3]-=mean[0];
		lorenz_coords[i*3+1]-=mean[1];
		lorenz_coords[i*3+2]-=mean[2];
	}

	for (i=0; i<num_precomputed_points; i++)
	{
		if (lorenz_coords[i*3]>max[0])
			max[0]=lorenz_coords[i*3];
		if (lorenz_coords[i*3+1]>max[1])
			max[1]=lorenz_coords[i*3+1];
		if (lorenz_coords[i*3+2]>max[2])
			max[2]=lorenz_coords[i*3+2];
	}

	float m= max[0];
	if (m<max[1])
		m=max[1];
	if (m<max[2])
		m=max[2];

	m=2;
	for (i=0; i<num_precomputed_points; i++)
	{
		lorenz_coords[i*3]/=m;
		lorenz_coords[i*3+1]/=m;
		lorenz_coords[i*3+2]/=m;
	}
}

void cleanup_arrays()
{
	if (lorenz_coords)
		free(lorenz_coords);
	if (lorenz_path)
		free(lorenz_path);

	if (satellite_times)
		free(satellite_times);
	if (satellite_speeds)
		free(satellite_speeds);
}

void callback(void)
{
	set_camera();
	display();

	simTime+=37*elapsedTime*camera_speed*num_points/num_precomputed_points;

	camera_angle+=37*elapsedTime*camera_angle_anim_speed;
	if (camera_angle<camera_angle_anim[0] || camera_angle>camera_angle_anim[1])
	{
		camera_angle_anim_speed*=-1;
		camera_angle+=camera_angle_anim_speed;
	}
}

void hack_draw (xstuff_t * XStuff, double currentTime, float frameTime)
{
	static float times[10] = { 0.03f, 0.03f, 0.03f, 0.03f, 0.03f,
		0.03f, 0.03f, 0.03f, 0.03f, 0.03f
	};
	static int timeindex = 0;
#ifdef BENCHMARK
	static int a = 1;
#endif

	if (frameTime > 0)
		times[timeindex] = frameTime;
	else
		times[timeindex] = elapsedTime;

#ifdef BENCHMARK
	elapsedTime = 0.027;
#else
	elapsedTime = 0.1f * (times[0] + times[1] + times[2] + times[3] + times[4] + times[5] + times[6] + times[7] + times[8] + times[9]);

	timeindex++;
	if (timeindex >= 10)
		timeindex = 0;
#endif

	callback();

#ifdef BENCHMARK
	if (a++ == 1000)
		exit(0);
#endif
}

void hack_reshape (xstuff_t * XStuff)
{
#ifdef DEBUG
	printf("hack_reshape\n");
#endif
	int w=XStuff->windowWidth;
	int h=XStuff->windowHeight;
	reshape(w,h);
}

void hack_init (xstuff_t * XStuff)
{
#ifdef DEBUG
	printf("hack_init\n");
#endif
	simTime=num_points/3;
	precompute_lorenz_array();
	reduce_points(num_points_max);
	init_satellites();
	init_line_strip();
	hack_reshape (XStuff);
}

void hack_cleanup (xstuff_t * XStuff)
{
#ifdef DEBUG
	printf("hack_cleanup\n");
#endif
	cleanup_arrays();
}

void hack_handle_opts (int argc, char **argv)
{
#ifdef DEBUG
	printf("handle_hack_opts\n");
#endif
	while (1) {
		int c;

#ifdef HAVE_GETOPT_H
		static struct option long_options[] = {
			{"help", 0, 0, 'h'},
			DRIVER_OPTIONS_LONG
			{"num-points", 1, 0, 'n'},
			{"num-satellites", 1, 0, 's'},
			{"camera-speed", 1, 0, 'c'},
			{"camera-angle", 1, 0, 'a'},
			{"line-width", 1, 0, 'l'},
			{"line-width-sat", 1, 0, 'w'},
			{"line-cutoff", 1, 0, 'o'},
		};

		c = getopt_long (argc, argv, DRIVER_OPTIONS_SHORT "hn:s:c:a:l:w:o:", long_options, NULL);
#else
		c = getopt (argc, argv, DRIVER_OPTIONS_SHORT "hn:s:c:a:l:w:o:");
#endif
		if (c == -1)
			break;

		switch (c) {
		DRIVER_OPTIONS_CASES case 'h':
			printf ("%s:"
#ifndef HAVE_GETOPT_H
				" Not built with GNU getopt.h, long options *NOT* enabled."
#endif
				"\n" DRIVER_OPTIONS_HELP
				"\t--num-points/-n <arg>\n"
				"\t--num-satellites/-s\n"
				"\t--camera-speed/-c\n"
				"\t--camera-angle/-a\n"
				"\t--line-width/l\n"
				"\t--line-width-sat/w\n"
				"\t--line-cutoff/o\n", argv[0]);
			exit (1);
		case 'n':
			num_precomputed_points = strtol_minmaxdef (optarg, 10, 100, 1000000, 1, num_points_default, "--num-points: ");
			break;
		case 's':
			num_satellites = strtol_minmaxdef (optarg, 10, 0, 50, 1, num_satellites_default, "--num-satellites: ");
			break;
		case 'c':
			camera_speed = 0.01 * strtol_minmaxdef (optarg, 10, 0, 100, 1, camera_speed_default/0.01, "--camera-speed: ");
			break;
		case 'a':
			camera_angle = strtol_minmaxdef (optarg, 10, 5, 179, 1, camera_angle_default, "--camera-angle: ");
			camera_angle_anim_speed=0;
			break;
		case 'l':
			line_width_attractor = strtol_minmaxdef (optarg, 10, 1, 100, 1, line_width_attractor_default, "--line-width: ");
			break;
		case 'w':
			line_width_satellites = strtol_minmaxdef (optarg, 10, 1, 100, 1, line_width_satellites_default, "--line-width-sat: ");
			break;
		case 'o':
			linear_cutoff = 0.01 * strtol_minmaxdef (optarg, 10, 0, 10000, 1, linear_cutoff_default/0.01, "--line-cutoff: ");
			break;
		}
	}
}
