### PRIORITY# ##
- handle 3 move repition

- handle stalemate in separate function (now it isn't handled, commented in checkWin)

- create a value_t (typedef)


- checking ai time

## IF TIME ##
- idea to remember last move effiently: each node remembers up to 3 cells that change their status 
    moved[3][3] --> [3] changed moves (-1 of not changed)
                    [3] x, y, player

- to check possible moves, you shound't have to look at all pieces, just the ones that are affected by the (up to) 3 cells that changed state

- currently model and human share the same movePiece, while easier to implement, I should make them seperate and the models one a lot more efficient


# WHO CARES #
- change name of negamarx func

- mouse and keyboard

- should free memory at some point

- colors

- move position of the board

- using memcpy more