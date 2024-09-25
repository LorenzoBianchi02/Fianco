### PRIORITY# ##
- more TT elements

- handle 3 move repition (I could just check in the root, and save the real last moves played)

- if you only have 1 move do it immidiatly

- checking ai time

- confirm ai move

- define for board->turn % 2 + 1

- windowing

- evaluation: punnish stack doubling


- end game DB
- opening book

## IF TIME ##
- idea to remember last move effiently: each node remembers up to 3 cells that change their status 
    moved[3][3] --> [3] changed moves (-1 of not changed)
                    [3] x, y, player
- to check possible moves, you shound't have to look at all pieces, just the ones that are affected by the (up to) 3 cells that changed state

- currently model and human share the same movePiece, while easier to implement, I should make them seperate and the models one a lot more efficient

- pondering

- better transposition replacement scheme

# WHO CARES #
- change name of negamarx func

- mouse and keyboard

- should free memory at some point

- colors

- move position of the board

- using memcpy more