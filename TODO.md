### PRIORITY# ##
- check valide move (you have to capture)

- put turn in board

- handle a player having no pieces
- handle a piece reaching the end of the board (win)
- handle stalemate


- handle 3 move repition

## IF TIME ##
- idea to remember last move effiently: each node remembers up to 3 cells that change their status 
    moved[3][3] --> [3] changed moves (-1 of not changed)
                    [3] x, y, player

- to check possible moves, you shound't have to look at all pieces, just the ones that are affected by the (up to) 3 cells that changed state

- currently model and human share the same movePiece, while easier to implement, I should make them seperate and the models one a lot more efficient


# WHO CARES #
- mouse and keyboard

- should free memory at some point

- colors

- move position of the board

- using memcpy more