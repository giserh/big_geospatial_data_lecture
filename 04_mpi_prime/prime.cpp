#include<iostream>
#include<cmath>
#include<vector>
using std::cout;
using std::endl;

// compile using   g++ -Ofast -std=c++11 -o prime prime.cpp
// or with OpenMP: g++ -fopenmp -Ofast -std=c++11 -o prime prime.cpp

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
    size_t l = 1000;  // how many primes

    if (argc == 2)
    {
        l = atoi(argv[1]);
    }

    #pragma omp parallel for
    for (size_t i=2; i < l; i++)
    {
         if (isprime(i)){
            #pragma omp critical
             primes.push_back(i);
         }
    }
    	if ( l < 10000) {
              for (auto &r:primes) cout << r << " ";
	  cout << endl;
	}else{
          cout << "Omit output as l is large." << endl;
	}

    return 0;
}
