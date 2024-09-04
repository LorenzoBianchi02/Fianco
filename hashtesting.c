#include <stdio.h>
#include <string.h>
#include "hashmap.h"

struct user {
    __uint128_t board;
    int val;
};

int user_compare(const void *a, const void *b, void *udata) {
    const struct user *ua = a;
    const struct user *ub = b;
    return ua->board - ub->board;
}

uint64_t user_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const struct user *user = item;
    return hashmap_murmur(&user->board, sizeof(uint64_t), seed0, seed1);
}

int main() {
    // create a new hash map where each item is a `struct user`. The second
    // argument is the initial capacity. The third and fourth arguments are 
    // optional seeds that are passed to the following hash function.
    struct hashmap *map = hashmap_new(sizeof(struct user), 0, 0, 0, 
                                     user_hash, user_compare, NULL, NULL);

    // Here we'll load some users into the hash map. Each set operation
    // performs a copy of the data that is pointed to in the second argument.
    hashmap_set(map, &(struct user){ .board=5, .val=1 });
    hashmap_set(map, &(struct user){ .board=7, .val=2 });
    hashmap_set(map, &(struct user){ .board=100, .val=3 });

    const struct user *user; 
    
    printf("\n-- get some users --\n");
    user = hashmap_get(map, &(struct user){ .board=5 });
    printf("val=%d\n", user->val);

    user = hashmap_get(map, &(struct user){ .board=7 });
    printf("val=%d\n", user->val);

    user = hashmap_get(map, &(struct user){ .board=100 });
    printf("val=%d\n", user->val);

    user = hashmap_get(map, &(struct user){ .board=1 });
    printf("%s\n", user?"exists":"not exists");

    printf("\n-- iterate over all users (hashmap_iter) --\n");
    size_t iter = 0;
    void *item;
    while (hashmap_iter(map, &iter, &item)) {
        const struct user *user = item;
        printf("(val=%d)\n", user->val);
    }

    hashmap_free(map);
}

// output:
// -- get some users --
// Jane val=47
// Roger val=68
// Dale val=44
// not exists
// 
// -- iterate over all users (hashmap_scan) --
// Dale (val=44)
// Roger (val=68)
// Jane (val=47)
//
// -- iterate over all users (hashmap_iter) --
// Dale (val=44)
// Roger (val=68)
// Jane (val=47)
