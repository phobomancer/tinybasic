# tinybasic
an implementation of tiny basic in C

## What
This is an implementation of the base requirements of tinybasic based on this
formal grammar:
```
    line ::= number statement CR | statement CR
 
    statement ::= PRINT expr-list
                  IF expression relop expression THEN statement
                  GOTO expression
                  INPUT var-list
                  LET var = expression
                  GOSUB expression
                  RETURN
                  CLEAR
                  LIST
                  RUN
                  END
 
    expr-list ::= (string|expression) (, (string|expression) )*
 
    var-list ::= var (, var)*
 
    expression ::= (+|-|ε) term ((+|-) term)*
 
    term ::= factor ((*|/) factor)*
 
    factor ::= var | number | (expression)
 
    var ::= A | B | C ... | Y | Z
 
    number ::= digit digit*
 
    digit ::= 0 | 1 | 2 | 3 | ... | 8 | 9
 
    relop ::= < (>|=|ε) | > (<|=|ε) | =

    string ::= " ( |!|#|$ ... -|.|/|digit|: ... @|A|B|C ... |X|Y|Z)* "
```
(as seen on wikipedia)
This is an incredibly limited version of BASIC even by BASIC standards...
as you can see only 12 KEYWORDS (and I will probably skip implementing `CLEAR`
Only one statement per line (with the exception in the case of 
`IF..THEN <statement>`.)
No loop constructs other that `GOTO`
Only 26 possible variables (A-Z) (and they're all integers)
Only 4 arithmatic operators (+,-,*,/)

At the time of this writing only 5 keywords have been implemented:
`PRINT`, `LET`, `GOTO`, `LIST`, and `RUN`

## Why
There are really two "Why"s here, why am I writing this, and why 
am I writing this so poorly.

First of this is sort of a test run for implementing TinyBasic
on a much more limited processor in pure assembly.

And that's part of the reason the code here might seem terrible.
Since this is ment to be reimplemented in assembly i'm choosing to ignore some
C features that might make the job easier, or at least neater.

Specifically, no structs, (or unions for that matter), no dynamic allocation,
and no C library functions unless strictly necessary (at the moment I have used
three C library functions printf()/putchar() for the PRINT function and fgets()
for user input.

## Jesus Christ! Why is everything a cast to unsigned short pointers!?
This is the result of using a char array as a static block of memory
to store and execute the program. Each line is stored in the memory block
as linenumber,link to the next line in the program (not necessarily the 
next to be executed, just the next line in order by line number). The line
number and link are stored as 16 bit unsigned integers. The program is 
BASIC-ally a kinda weird linked list, but the link is an array index rather 
than a memory address. This is where all the casts to unsigned short comes 
from. (Yes I know I should use uint16_t, and I might in the future but for now
this is what we have)

