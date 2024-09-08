### PRIORITY# ##
- check valide move (you have to capture)
- player move (turns)

- idea to remember last move effiently: each node remembers up to 3 cells that change their status 
    moved[3][3] --> [3] changed moves (-1 of not changed)
                    [3] x, y, player


- handle a player having no pieces
- handle a piece reaching the end of the board (win)


## IF TIME ##
- lets not use a list for the linked hashmap, but just 2 seperate arrays, that way the hashfunction can just take in a piece_t and the pointers aren't a problem anymore

- change list_elem name to piece

- move position of the board

- mouse and keyboard

- remove hashtesting.c


# WHO CARES #
- should free memory at some point

- colors