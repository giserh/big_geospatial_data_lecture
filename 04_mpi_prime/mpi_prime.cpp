#include<iostream>
#include<cmath>
#include<vector>

using std::cout;
using std::endl;

#include "mpi.h"
// returns true if and only if number is a prime.
bool isprime(int number)
{
   for (size_t i=2; i<=sqrt(number); i++)
   {
      if (number % i == 0)
        return false;
   }
   return true;
}

std::vector<int> primes;


int main( int argc, char *argv[] )
{
    int rank;
    int size;
    size_t l = 1000;  // how many primes
    
    MPI_Init( 0, 0 );
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int my_start = 0; // change according to assignment
    int my_end = 0;
    if (rank == 0) my_start = 2; // 0 and 1 are not interesting.
    
    printf( "Hello world from process %d of %d\nFeeling responsible for %d..%d\n", rank, size, my_start, my_end );
    

   
    MPI_Finalize();

    
    
return 0;
}
