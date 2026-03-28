#include <bits/stdc++.h>
using namespace std;
int main() {
    int a = 1;
    int b = 2;
    a += b;
    return 0;
}

/*

 Performance counter stats for './a.out':

              1.60 msec task-clock:u                     #    0.768 CPUs utilized             
                 0      context-switches:u               #    0.000 /sec                      
                 0      cpu-migrations:u                 #    0.000 /sec                      
               121      page-faults:u                    #   75.819 K/sec                     
         1,663,029      cycles:u                         #    1.042 GHz                       
            20,345      stalled-cycles-frontend:u        #    1.22% frontend cycles idle      
            45,095      stalled-cycles-backend:u         #    2.71% backend cycles idle       
         1,756,511      instructions:u                   #    1.06  insn per cycle            
                                                  #    0.03  stalled cycles per insn   
           311,765      branches:u                       #  195.354 M/sec                     
             9,919      branch-misses:u                  #    3.18% of all branches           

       0.002079298 seconds time elapsed

       0.000000000 seconds user
       0.002358000 seconds sys

*/