#include <cstdlib>
#include <cstdio>
#include <cmath>

#include <list>

#include "bin.h"

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



int BinArray::Assign (particle_t& particle)
{
  int bin_x = (int) floor(particle.x / cutoff);
  int bin_y = (int) floor(particle.y / cutoff);

  return (bins + bin_x + bin_y * dim_bins)->Assign (particle);
}

void BinArray::Refresh ()
{
  for (int k = 0; k < dim_bins * dim_bins; ++k)
    (bins + k)->Flush();
}

void BinArray::Bin::Mark (const BinArray::Bin* bin)
{
  markings.push_back(const_cast<BinArray::Bin*>(bin));
}


/* NOTE: if I bin and THEN evaluate forces, I can get by with .... */


int BinArray::Bin::Assign (particle_t& new_guy)
{
  /* Acquire a lock on this bin, and clockwise around all neighbors */

  /* If lock is unavailable, return failure */
  if ( 0 /* Fail */ )
    return -1;

  // Compute forces with other particles in this bin, if already assigned
    /* This section requires a write lock on this bin's particles */
  if ( !particles.empty() )
    for ( std::list<particle_t*>::iterator i = particles.begin() ;
	i != particles.end();
	++i )
    {
      apply_force ( **i, new_guy );
      apply_force ( new_guy, **i );
    }

  // Add this particle to local list;
  particles.push_back(&new_guy);

  // Mark neighbors
  for (int i = 0; i < 9; ++i)
    if (i != 4)
    {
      (*(neighbors + i))->Mark(this);
      /* No longer need a write lock on this neighbor's markings.
       * Still need a write lock on their particles */
    } /* Still need a write lock on my own particles and markings */

  if ( !markings.empty() )
  {
    
    // Compute forces _from_ other particles in adjacent bins
    for ( std::list<Bin*>::iterator b = markings.begin() ;
	b != markings.end();
	++b )
      /* Bin b's particles must be locked */
      for ( std::list<particle_t*>::iterator i = (*b)->particles.begin() ;
	  i != (*b)->particles.end();
	  ++i )
	/* My particles must be locked too */
	apply_force ( new_guy, **i );

  /* Release write lock on this bin's particles - no more changes to
   * new_guy from this call. 
   * Maintain lock on my markings */
  
    
    // Compute forces _on_ other particles in adjacent bins
    for ( std::list<Bin*>::iterator b = markings.begin() ;
	b != markings.end();
	++b )
      /* Bin b's particles must still be locked */
      for ( std::list<particle_t*>::iterator i = (*b)->particles.begin() ;
	  i != (*b)->particles.end();
	  ++i )
      {
	apply_force ( **i, new_guy );
      }
      /* Release lock on b's particles */

   }

  /* Release lock on my markings */

  /* Since we hold a lock on b's particles and my markings until the end, 
   * probably better not to bother with random unlockings - not much can
   * really happen */

  return 0;
}

void BinArray::Bin::Flush()
{
  particles.clear();
  markings.clear();
}
