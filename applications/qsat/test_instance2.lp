% Expect 1 solution
length(3).
or(0).
and(1).
or(2).

atom(1,a).
atom(2,b).
atom(3,c).

clause(c0).
clause(c1).
clause(c2).
clause(c3).

pos(c0,a).
pos(c0,b).
neg(c0,c).

pos(c1,b).
pos(c1,c).

pos(c2,a).
neg(c2,b).
pos(c2,c).

neg(c3,a).
neg(c3,b).
neg(c3,c).
