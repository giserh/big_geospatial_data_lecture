#include<iostream>
#include<cmath>
#include<vector>
#include<stdint.h>

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

std::vector<int32_t> primes;


int main( int argc, char *argv[] )
{
    int rank;
    int size;
    size_t l = 1000;  // how many primes

    if (argc == 2)
    {
        l = atoi(argv[1]);
    }
    
    MPI_Init( 0, 0 );
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (rank == 0){
        cout << "l=" << l << endl;
    }
    
    int my_start = rank * (l/size);
    int my_end = (rank+1) * (l/size) -1;
    if (rank == 0) my_start = 2; // 0 and 1 are not interesting.
     MPI_Barrier(MPI_COMM_WORLD); // just to have clean output    
    
    for (size_t i=my_start; i <= my_end; i++)
    {
         if (isprime(i))
	   primes.push_back(i);   
    }
    // now every processor has a local array of primes. Lets output the count
    cout << "Processor " << rank << "/" << size << " found " << primes.size() << " prime numbers."  << endl;
    int local_count, global_count;
    local_count = primes.size();
    // reduce the number of primes and allocate  
    MPI_Reduce(&local_count, &global_count, 1, MPI_INT, MPI_SUM,
	       0, MPI_COMM_WORLD);

     std::vector<int32_t> result;
     if (rank == 0)
     {
         cout << "all processors found " << global_count << " primes." << endl;
	 result.resize(global_count);
	 // just for convenience, that we see what has been written
	 for (auto &r: result) r = -1;
     }


     // gather the individual numbers
     std::vector<int> nums(size);
     MPI_Gather(&local_count,1,MPI_INT,&nums[0],1,MPI_INT,0,MPI_COMM_WORLD);
     if (rank == 0) for (size_t i=0; i < size;i++) cout << i << ":"<< nums[i] << "\n" ;

     std::vector<int> disps(size);
    // Displacement for the first chunk of data - 0
    for (int i = 0; i < size; i++)
       disps[i] = (i > 0) ? (disps[i-1] + nums[i-1]) : 0;
     
     // gather all primes.
     MPI_Gatherv(
	 &primes[0], local_count, MPI_INT,
	 &result[0],&nums[0],&disps[0],MPI_INT,
	 0,MPI_COMM_WORLD);
     
     if (rank == 0)
     {
        cout << "received: " << result.size() << endl;
	if ( l < 10000) {
	  for (auto &r:result) cout << r << " ";
	  cout << endl;
	}else{
          cout << "Omit output as l is large." << endl;
	}
     } 
    
    

   
    MPI_Finalize();

    
    
return 0;
}
