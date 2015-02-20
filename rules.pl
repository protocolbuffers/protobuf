#!/usr/bin/perl -w

submit_rule(submit(CR, V)) :-
  CR = label('Code-Review', ok(_)),
  V = label('Verified', ok(_)).
