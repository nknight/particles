#include <cstdlib>
#include <cstdio>
#include <cmath>

#include <list>

#include "bin_shared.h"

BinArray::~BinArray ()
{
  delete [] bins;
  delete nil;
}

BinArray::BinArray (double size, double cutoff)
{
  this->size = size;
  this->cutoff = cutoff;

  dim_bins = (int) ( ceil(size/cutoff) );
  bins = new Bin[ dim_bins * dim_bins ];

    nil = new Bin;

  Bin* cur = NULL;
  // Make neighbors
  for (int j = 0; j < dim_bins; ++j)
    for (int i = 0; i < dim_bins; ++i)
    {
      cur = (bins + i + j * dim_bins);

      if (i == 0 || j == 0)
	cur->neighbors[0] = nil;
      else
	cur->neighbors[0] = (bins + (i-1) + (j-1)*dim_bins);

      if (j == 0)
	cur->neighbors[1] = nil;
      else
	cur->neighbors[1] = (bins + (i-0) + (j-1)*dim_bins);

      if (i == dim_bins-1 || j == 0)
	cur->neighbors[2] = nil;
      else
	cur->neighbors[2] = (bins + (i+1) + (j-1)*dim_bins);

      if (i == 0)
	cur->neighbors[3] = nil;
      else
	cur->neighbors[3] = (bins + (i-1) + (j-0)*dim_bins);

      cur->neighbors[4] = cur;

      if (i == dim_bins-1)
	cur->neighbors[5] = nil;
      else
	cur->neighbors[5] = (bins + (i+1) + (j-0)*dim_bins);

      if (i == 0 || j == dim_bins-1)
	cur->neighbors[6] = nil;
      else
	cur->neighbors[6] = (bins + (i-1) + (j+1)*dim_bins);

      if (j == dim_bins-1)
	cur->neighbors[7] = nil;
      else
	cur->neighbors[7] = (bins + (i-0) + (j+1)*dim_bins);

      if (i == dim_bins-1 || j == dim_bins-1)
	cur->neighbors[8] = nil;
      else
	cur->neighbors[8] = (bins + (i+1) + (j+1)*dim_bins);
    }
}

BinArray* BinArray::p_instance = NULL;

BinArray* BinArray::Instance (double size, double cutoff)
{
  if (p_instance == NULL)
    p_instance = new BinArray ( size, cutoff );

  return p_instance;
}

BinArray* BinArray::Destroy ()
{
  if (p_instance != NULL)
    delete p_instance;

  p_instance = NULL;

  return p_instance;
}

void BinArray::Assign (particle_t& particle)
{
  int bin_x = (int) floor(particle.x / cutoff);
  int bin_y = (int) floor(particle.y / cutoff);

  //fprintf (stderr, " attempting to put a particle in bin row %d col %d\n", bin_x, bin_y);
  (bins + bin_x + bin_y * dim_bins)->Assign (particle);
}

void BinArray::Refresh ()
{
  for (int k = 0; k < dim_bins * dim_bins; ++k)
    (bins + k)->Flush();
}

BinArray::Bin::Bin()
{
  pthread_mutexattr_init (&attr);
  pthread_mutex_init (&lock, &attr);
}

BinArray::Bin::~Bin()
{
  pthread_mutexattr_destroy (&attr);
  pthread_mutex_destroy (&lock);
}
    

void BinArray::Bin::Assign (particle_t& new_guy)
{
  pthread_mutex_lock ( &(this->lock) );

  // Add this particle to local list;
  particles.push_back(&new_guy);

  pthread_mutex_unlock ( &(this->lock) );
}

void BinArray::DoForces (particle_t& me)
{

  int bin_x = (int) floor(me.x / cutoff);
  int bin_y = (int) floor(me.y / cutoff);

  Bin *this_bin = bins + bin_x + bin_y * dim_bins;

  //fprintf (stderr, " attempting to put a particle in bin row %d col %d\n", bin_x, bin_y);

//  pthread_mutex_lock ( &(this_bin->lock) );
  for (int i = 0; i < 9; ++i)
    for ( std::list<particle_t*>::iterator itr = (*(this_bin->neighbors + i))->particles.begin() ;
	itr != (*(this_bin->neighbors + i))->particles.end();
	++itr )
      apply_force ( me, **itr );

//  pthread_mutex_unlock ( &(this_bin->lock) );
}

void BinArray::Bin::Flush()
{  
  particles.clear();
}
