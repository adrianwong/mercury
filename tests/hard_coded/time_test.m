:- module time_test.

:- interface.

:- import_module io.

:- pred main(io__state::di, io__state::uo) is det.

:- implementation.

:- import_module std_util, time.

main -->
	time(Time),
	( { mktime(localtime(Time)) = Time } ->
		io__write_string("mktime succeeded\n")
	;
		io__write_string("mktime failed\n")
	),
	
	% Sunday 2001-01-07 03:02:01
	{ TM = tm(1, 2, 3, 0, 7, 6, 0, 101, no) },
	io__write_string(asctime(TM)),
	io__nl.

