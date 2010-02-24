#include <cstdlib>
#include <cstdio>
#include <deque>
#include <pthread.h>

#include "common.h"
#include "bin_shared.h"
//#include "pthread_barrier.h"

extern double size;
double cutoff = 0.01;

void* do_sim ( void* );

#define P( condition ) {if( (condition) != 0 ) { fprintf( stderr, "\n FAILURE in %s, line %d\n", __FILE__, __LINE__ );exit( 1 );}}

//
//  global variables
//
int n, n_threads;
particle_t *particles;
FILE *fsave;
pthread_barrier_t barrier;
BinArray *Bins;
std::deque<particle_t*> worklist;

int main( int argc, char **argv )
{    
    //
    //  process command line
    //
    if( find_option( argc, argv, "-h" ) >= 0 )
    {
        printf( "Options:\n" );
        printf( "-h to see this help\n" );
        printf( "-n <int> to set the number of particles\n" );
        printf( "-p <int> to set the number of threads\n" );
        printf( "-o <filename> to specify the output file name\n" );
        return 0;
    }
    
    n = read_int( argc, argv, "-n", 1000 );
    n_threads = read_int( argc, argv, "-p", 2 );
    char *savename = read_string( argc, argv, "-o", NULL );
    
    //
    //  allocate resources
    //
    fsave = savename ? fopen( savename, "w" ) : NULL;

    particles = (particle_t*) malloc( n * sizeof(particle_t) );
    set_size( n );
    init_particles( n, particles );

    Bins = BinArray::Instance( size, cutoff );
    
    pthread_attr_t attr;
    P( pthread_attr_init( &attr ) );
    P( pthread_barrier_init( &barrier, NULL, n_threads ) );

    int *thread_ids = (int *) malloc( n_threads * sizeof( int ) );
    for( int i = 0; i < n_threads; i++ ) 
        thread_ids[i] = i;

    pthread_t *threads = (pthread_t *) malloc( n_threads * sizeof( pthread_t ) );
    
    //
    //  do the parallel work
    //
    double simulation_time = read_timer( );
    for( int i = 1; i < n_threads; i++ ) 
        P( pthread_create( &threads[i], &attr, &do_sim, &thread_ids[i] ) );
    
    do_sim( &thread_ids[0] );
    
    for( int i = 1; i < n_threads; i++ ) 
        P( pthread_join( threads[i], NULL ) );
    simulation_time = read_timer( ) - simulation_time;
    
    fprintf(stderr, "n = %d, n_threads = %d, simulation time = %g seconds\n", n, n_threads, simulation_time );
    
    //
    //  release resources
    //
    P( pthread_barrier_destroy( &barrier ) );
    P( pthread_attr_destroy( &attr ) );
    free( thread_ids );
    free( threads );
    free( particles );
    Bins = BinArray::Destroy();
    if( fsave )
        fclose( fsave );
    
    return 0;
}

void* do_sim ( void* pthread_id )
{
  int thread_id = *(int*)pthread_id;

  int particles_per_thread = (n + n_threads - 1) / n_threads;
  int first = min(  thread_id    * particles_per_thread, n );
  int last  = min( (thread_id+1) * particles_per_thread, n );

  fprintf(stderr, " % d %d... \n", first, last);

//fprintf(stderr, "NSTEPS: %d\n", NSTEPS);
  for( int step = 0; step < NSTEPS; step++ )
  {
    if ( thread_id == 0 )
    {
      worklist.clear();
      Bins->Refresh();
    }
 pthread_barrier_wait(&barrier);
  // fprintf(stderr, " % d: iteration %d, gonna sweep particles... \n", thread_id, step);
    particle_t* cur = particles + first;
    for (int i = first; i < last; ++i)
    {
      //fprintf(stderr, "particles=%p, particles+n=%p, this one=%p\n", particles, particles + n-1, cur);

      cur->ax = cur->ay = 0.;

      // Bin current particle
      if (Bins->Assign(*cur) == -1)
      {
	// TODO: lock worklist
//	worklist.push_back(cur);
	// TODO: unlock worlist
      }

      ++cur;
    }

    pthread_barrier_wait( &barrier );
    
    if ( thread_id == 0)
    {
      while (!worklist.empty())
      {
	cur = worklist.front();
	Bins->Assign(*cur);
	worklist.pop_front();
      }
    }

   //fprintf(stderr, " % d: iteration %d, waiting at second barrier \n", thread_id, step);
    pthread_barrier_wait( &barrier );
    
    cur = particles + first;
    for (int i = first; i < last; ++i)
    {
      move (*cur);
      ++cur;
    }

  // fprintf(stderr, " % d: iteration %d, waiting at third barrier \n", thread_id, step);
    pthread_barrier_wait( &barrier );
    
    if ( thread_id == 0 ) 
      if( fsave && (step%SAVEFREQ) == 0 )
	save( fsave, n, particles );

  }
  return NULL;
}
