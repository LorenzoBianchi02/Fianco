#include <stdio.h>
#include <stdlib.h>

typedef struct state{
    __uint128_t white;
    __uint128_t black;
    
    int eval;
}state;


int main(){
    state st = {.white = 11, .black = 0, .eval = 5};

    for(int i=0; i<10; i++, st.white = st.white >> 1){
        if(st.white >> 1)
            printf("1");
        else
            printf("0");

    }
}
