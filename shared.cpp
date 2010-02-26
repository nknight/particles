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
pthread_mutex_t lock;
BinArray *Bins;

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

    pthread_mutexattr_t mattr;
    pthread_mutexattr_init (&mattr);
    pthread_mutex_init (&lock, &mattr);


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
    
    pthread_mutexattr_destroy (&mattr);
    pthread_mutex_destroy (&lock);
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

  for( int step = 0; step < NSTEPS; step++ )
  {
    
    if ( thread_id == 0 )
      Bins->Refresh();
    pthread_barrier_wait(&barrier);
    /* Note - could eliminate this barrier by marking each bin as "stale"
     * during the DoForces() loop, and then clearing each "stale" bin as
     * a part of the Assign() logic, before adding a new particle). This
     * optimization would obviate the need to have this barrier that has
     * idle threads waiting for Refresh() to return to thread 0.
     *
     * This change would require additional logic in DoForces(), so that 
     * when you dutifully check each neighbor, and it contains stale
     * data, that you don't include it in your calculation. 
     *
     * However, this would require either (a) really careful
     * synchronization in the DoForces() loop, or (b) another for
     * loop over particles to set the "stale" flag -- although the
     * latter could be done recklessly, without locks, since it would
     * be a race condition between the same value. */

    // fprintf(stderr, " % d: iteration %d, gonna sweep particles... \n", thread_id, step);
    
    particle_t* cur = particles + first;
    for (int i = first; i < last; ++i)
    {
      //fprintf(stderr, "particles=%p, particles+n=%p, this one=%p\n", particles, particles + n-1, cur);

      cur->ax = cur->ay = 0.;

      // Bin current particle
      Bins->Assign(*cur);

      ++cur;
    }

    pthread_barrier_wait( &barrier );

    cur = particles + first;
    for (int i = first; i < last; ++i)
    {
      //fprintf(stderr, "particles=%p, particles+n=%p, this one=%p\n", particles, particles + n-1, cur);

      // Calculate forces on current particle
      Bins->DoForces(*cur);

      ++cur;
    }

    pthread_barrier_wait( &barrier );
 
    cur = particles + first;
    for (int i = first; i < last; ++i)
    {

      // Calculate forces on current particle
      move(*cur);

      ++cur;
    }

    pthread_barrier_wait( &barrier );
    
    if ( thread_id == 0 ) 
      if( fsave && (step%SAVEFREQ) == 0 )
	save( fsave, n, particles );

  }
  return NULL;
}
