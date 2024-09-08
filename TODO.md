### PRIORITY# ##
- check valide move (you have to capture)

- idea to remember last move effiently: each node remembers up to 3 cells that change their status 
    moved[3][3] --> [3] changed moves (-1 of not changed)
                    [3] x, y, player


- handle a player having no pieces
- handle a piece reaching the end of the board (win)

- use coords (either just x, y; or even fromx/fromy/tox/toy)


## IF TIME ##
- currently model and human share the same movePiece, while easier to implement, I should make them seperate and the models one a lot more efficient

- lets not use a list for the linked hashmap, but just 2 seperate arrays, that way the hashfunction can just take in a piece_t and the pointers aren't a problem anymore

- change list_elem name to piece

- move position of the board

- mouse and keyboard

- remove hashtesting.c

- to check possible moves, you shound't have to look at all pieces, just the ones that are affected by the (up to) 3 cells that changed state


# WHO CARES #
- should free memory at some point

- colors