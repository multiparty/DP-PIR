#include <string>
#include <iostream>
#include <cstdlib>

int z = 9857;

unsigned int mod(int a, int modulus){
  unsigned int rem = a % modulus > 0 ? a : a + modulus;
  return rem;
}


unsigned int** generateShares(unsigned int query, unsigned int numparty){

  unsigned int** shares = (unsigned int **) malloc(2* sizeof(unsigned int *));
  shares[0] = (unsigned int *) malloc(numparty * sizeof(unsigned int));
  shares[1] = (unsigned int *) malloc(numparty * sizeof(unsigned int));

  unsigned int t = 1;
  for (int i = 0; i < numparty-1; i++){
      shares[0][i] = std::rand() % z + 1;
      shares[1][i] = std::rand() % z + 1;
      t = (t * shares[1][i] + shares[0][i]) % z;
  }
  shares[1][numparty-1] = std::rand() % z + 1;
  shares[0][numparty-1] = mod(query - t*shares[1][numparty-1], z); // c++11: % of negative numbers is negative

  return shares;
}

unsigned int reconstructSecret(unsigned int** shares, unsigned int numparty){

  unsigned int query = 1;
  for(int i = 0; i < numparty; i++){
    query = (shares[1][i]*query + shares[0][i]) % z;
  }

  return query;
}

int main(){
  unsigned int q = 3;
  unsigned int numparty = 4;

  unsigned int** shares = generateShares(q, numparty);
  unsigned int query = reconstructSecret(shares, numparty);

  std::cout << "Query:" << q << std::endl;
  std::cout << "Query Reconstructed:" << query << std::endl;
}
