submit_rule(submit(CR)) :-
  base(CR),
  gerrit:current_user(Reviewer),
  gerrit:commit_author(Author),
  CR = label(_, ok(Reviewer)),
  Author \= Reviewer,
  !.
submit_rule(submit(CR, N)) :-
  base(CR),
  N = label('Non-Author-Code-Review', need(_)).
base(CR) :-
  gerrit:max_with_block(-2, 1, 'Code-Review', CR).
