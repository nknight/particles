#include <list>
#include <pthread.h>

#include "common.h"

class BinArray;

class BinArray
{

 public:

  /* Only this function can call the constructor. It returns a pointer to a static variable, so
   * it too must be static */
  static BinArray* Instance( double, double );
  static BinArray* Destroy ();

  /* Other methods */
  void Refresh();
  void Assign (particle_t&);
  void DoForces (particle_t&);

 private:
  class Bin
  {
   public:
    void Assign (particle_t&);
    void Flush ();

    Bin ();

    ~Bin();
    
    pthread_mutexattr_t attr;
    pthread_mutex_t lock;
    
    std::list<particle_t*> particles;

    Bin* neighbors[9];
  };

  ~BinArray ();
  BinArray () { }
  BinArray (double, double);
  BinArray (const BinArray&) { }
  BinArray& operator= (const BinArray&) { return *this; }

  double cutoff;
  double size;
  int dim_bins;

  Bin *bins;

  Bin *nil;

  static BinArray* p_instance;

};

