#include <cstdlib>
#include <cstdio>
#include <deque>

#include "common.h"
#include "bin.h"

extern double size;
double cutoff = 0.01;

int main (int argc, char** argv)
{
  if( find_option( argc, argv, "-h" ) >= 0 )
  {
    printf( "Options:\n" );
    printf( "-h to see this help\n" );
    printf( "-n <int> to set the number of particles\n" );
    printf( "-o <filename> to specify the output file name\n" );
    return 0;
  }

  int n = read_int( argc, argv, "-n", 1000 );

  char* savename = read_string( argc, argv, "-o", NULL );

  FILE* fsave = savename ? fopen( savename, "w" ) : NULL;

  particle_t *particles = (particle_t*) malloc( n * sizeof(particle_t) );

  set_size( n );

  init_particles( n, particles );

  BinArray *Bins = BinArray::Instance( size, cutoff );
  
  double simulation_time = read_timer( );
 
  for( int step = 0; step < NSTEPS; step++ )
  {

    // Clear out bins
    Bins->Refresh();

    std::deque<particle_t*> worklist;

    particle_t* cur = particles;
    for (int i = 0; i < n; ++i)
    {
      move (*cur);

      // Bin current particle
      if (Bins->Assign(*cur) == -1)
        worklist.push_back(cur);
      
      ++cur;
    }
    while (!worklist.empty())
    {
      cur = worklist.front();
      Bins->Assign(*cur);
      worklist.pop_front();
    }

    if( fsave && (step%SAVEFREQ) == 0 )
      save( fsave, n, particles );
  }
  simulation_time = read_timer( ) - simulation_time;

  printf( "n = %d, simulation time = %g seconds\n", n, simulation_time );

  free( particles );
  if( fsave )
    fclose( fsave );
  return 0;
}
