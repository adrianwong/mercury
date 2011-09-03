%-----------------------------------------------------------------------------%
% vim: ft=mercury ts=4 sw=4 et
%-----------------------------------------------------------------------------%
% Copyright (C) 2011 The University of Melbourne.
% This file may only be copied under the terms of the GNU General
% Public License - see the file COPYING in the Mercury distribution.
%-----------------------------------------------------------------------------%
%
% Test operations on bitsets by comparing the output with the output
% from an ordinary set.
%
%-----------------------------------------------------------------------------%

:- module test_bitset.

:- interface.

:- import_module enum.
:- import_module list.
:- import_module set.

:- type test_bitset(T).

:- type bitset_error(T)
    --->    zero_argument(string,
                test_bitset(T))
    ;       one_argument(string,
                test_bitset(T), test_bitset(T))
    ;       two_arguments(string,
                test_bitset(T), test_bitset(T), test_bitset(T)).

:- func init = test_bitset(T).
:- func singleton_set(T) = test_bitset(T) <= enum(T).
:- func make_singleton_set(T) = test_bitset(T) <= enum(T).

:- pred init(test_bitset(T)::out) is det.
:- pred singleton_set(test_bitset(T)::out, T::in) is det <= enum(T).
:- pred make_singleton_set(test_bitset(T)::out, T::in) is det <= enum(T).

:- func count(test_bitset(T)) = int <= enum(T).

%---------------
% Tests.

:- pred is_empty(test_bitset(T)::in) is semidet.
:- pred is_non_empty(test_bitset(T)::in) is semidet.
:- pred is_singleton(test_bitset(T)::in, T::out) is semidet <= enum(T).

:- pred contains(test_bitset(T)::in, T::in) is semidet <= enum(T).
:- pred member(T, test_bitset(T)) <= enum(T).
:- mode member(in, in) is semidet.
:- mode member(out, in) is nondet.

:- pred equal(test_bitset(T)::in, test_bitset(T)::in) is semidet <= enum(T).

:- pred subset(test_bitset(T)::in, test_bitset(T)::in) is semidet.
:- pred superset(test_bitset(T)::in, test_bitset(T)::in) is semidet.

%---------------
% Conversions.

:- func list_to_set(list(T)) = test_bitset(T) <= enum(T).
:- func sorted_list_to_set(list(T)) = test_bitset(T) <= enum(T).
:- func to_sorted_list(test_bitset(T)) = list(T) <= enum(T).

:- pred list_to_set(list(T)::in, test_bitset(T)::out) is det <= enum(T).
:- pred sorted_list_to_set(list(T)::in, test_bitset(T)::out) is det <= enum(T).
:- pred to_sorted_list(test_bitset(T)::in, list(T)::out) is det <= enum(T).

:- func set_to_bitset(set(T)) = test_bitset(T) <= enum(T).
:- func bitset_to_set(test_bitset(T)) = set(T) <= enum(T).
:- func from_set(set(T)) = test_bitset(T) <= enum(T).
:- func to_set(test_bitset(T)) = set(T) <= enum(T).

%---------------
% Updates.

:- pred insert(T::in, test_bitset(T)::in, test_bitset(T)::out)
    is det <= enum(T).
:- pred insert_list(list(T)::in, test_bitset(T)::in, test_bitset(T)::out)
    is det <= enum(T).
:- pred delete(T::in, test_bitset(T)::in, test_bitset(T)::out)
    is det <= enum(T).
:- pred delete_list(list(T)::in, test_bitset(T)::in, test_bitset(T)::out)
    is det <= enum(T).
:- pred remove(T::in, test_bitset(T)::in, test_bitset(T)::out)
    is semidet <= enum(T).
:- pred remove_list(list(T)::in, test_bitset(T)::in, test_bitset(T)::out)
    is semidet <= enum(T).
:- pred remove_least(T::out, test_bitset(T)::in, test_bitset(T)::out)
    is semidet <= enum(T).

%---------------
% Set operations.

:- func union(test_bitset(T), test_bitset(T)) = test_bitset(T) <= enum(T).
:- func union_list(list(test_bitset(T))) = test_bitset(T) <= enum(T).
:- func intersect(test_bitset(T), test_bitset(T)) = test_bitset(T) <= enum(T).
:- func intersect_list(list(test_bitset(T))) = test_bitset(T) <= enum(T).
:- func difference(test_bitset(T), test_bitset(T)) = test_bitset(T) <= enum(T).

:- pred union(test_bitset(T)::in,
    test_bitset(T)::in, test_bitset(T)::out) is det <= enum(T).
:- pred union_list(list(test_bitset(T))::in, test_bitset(T)::out) is det
    <= enum(T).
:- pred intersect(test_bitset(T)::in,
    test_bitset(T)::in, test_bitset(T)::out) is det <= enum(T).
:- pred intersect_list(list(test_bitset(T))::in, test_bitset(T)::out) is det
    <= enum(T).
:- pred difference(test_bitset(T)::in,
    test_bitset(T)::in, test_bitset(T)::out) is det <= enum(T).

:- pred divide(pred(T)::in(pred(in) is semidet), test_bitset(T)::in,
    test_bitset(T)::out, test_bitset(T)::out) is det <= enum(T).

:- pred divide_by_set(test_bitset(T)::in, test_bitset(T)::in,
    test_bitset(T)::out, test_bitset(T)::out) is det <= enum(T).

%---------------
% Traversals.

:- pred foldl(pred(T, Acc, Acc), test_bitset(T), Acc, Acc) <= enum(T).
:- mode foldl(pred(in, in, out) is det, in, in, out) is det.
:- mode foldl(pred(in, in, out) is semidet, in, in, out) is semidet.

:- func foldl(func(T, Acc) = Acc, test_bitset(T), Acc) = Acc <= enum(T).
:- mode foldl(func(in, in) = out is det, in, in) = out is det.

:- func filter(pred(T)::in(pred(in) is semidet), test_bitset(T)::in)
    = (test_bitset(T)::out) is det <= enum(T).
:- pred filter(pred(T)::in(pred(in) is semidet),
    test_bitset(T)::in, test_bitset(T)::out, test_bitset(T)::out)
    is det <= enum(T).

%-----------------------------------------------------------------------------%

:- implementation.

:- import_module bool.
:- import_module exception.
:- import_module int.
:- import_module list.
:- import_module maybe.
:- import_module pair.
:- import_module require.
:- import_module set_ordlist.
:- import_module solutions.
:- import_module string.
:- import_module tree_bitset.

:- type test_bitset(T) == pair(tree_bitset(T), set_ordlist(T)).

%-----------------------------------------------------------------------------%

init = tree_bitset.init - set_ordlist.init.

singleton_set(A) =
    tree_bitset.make_singleton_set(A) - set_ordlist.make_singleton_set(A).

make_singleton_set(A) =
    tree_bitset.make_singleton_set(A) - set_ordlist.make_singleton_set(A).

init(init).
singleton_set(test_bitset.singleton_set(A), A).
make_singleton_set(test_bitset.make_singleton_set(A), A).

count(SetA - SetB) = Count :-
    CountA = tree_bitset.count(SetA),
    CountB = set_ordlist.count(SetB),
    ( CountA = CountB ->
        Count = CountA
    ;
        error("test_bitset: count failed")
    ).

%-----------------------------------------------------------------------------%

is_empty(A - B) :-
    ( tree_bitset.is_empty(A) -> EmptyA = yes; EmptyA = no),
    ( set_ordlist.is_empty(B) -> EmptyB = yes; EmptyB = no),
    ( EmptyA = EmptyB ->
        EmptyA = yes
    ;
        error("test_bitset: is_empty failed")
    ).

is_non_empty(A - B) :-
    ( tree_bitset.is_non_empty(A) -> NonEmptyA = yes; NonEmptyA = no),
    ( set_ordlist.non_empty(B) -> NonEmptyB = yes; NonEmptyB = no),
    ( NonEmptyA = NonEmptyB ->
        NonEmptyA = yes
    ;
        error("test_bitset: is_non_empty failed")
    ).

is_singleton(A - B, E) :-
    ( tree_bitset.is_singleton(A, AE) -> NonEmptyA = yes(AE); NonEmptyA = no),
    ( set_ordlist.singleton_set(B, BE) -> NonEmptyB = yes(BE); NonEmptyB = no),
    ( NonEmptyA = NonEmptyB ->
        NonEmptyA = yes(E)
    ;
        error("test_bitset: is_singleton failed")
    ).

contains(SetA - SetB, E) :-
    ( tree_bitset.contains(SetA, E) -> InSetA = yes ; InSetA = no),
    ( set_ordlist.contains(SetB, E) -> InSetB = yes ; InSetB = no),
    ( InSetA = InSetB ->
        InSetA = yes
    ;
        error("test_bitset: contains failed")
    ).

:- pragma promise_equivalent_clauses(member/2).

member(E::in, (SetA - SetB)::in) :-
    ( tree_bitset.member(E, SetA) -> InSetA = yes ; InSetA = no),
    ( set_ordlist.member(E, SetB) -> InSetB = yes ; InSetB = no),
    ( InSetA = InSetB ->
        InSetA = yes
    ;
        error("test_bitset: member failed")
    ).

member(E::out, (SetA - SetB)::in) :-
    PredA = (pred(EA::out) is nondet :- tree_bitset.member(EA, SetA)),
    PredB = (pred(EB::out) is nondet :- set_ordlist.member(EB, SetB)),
    solutions(PredA, SolnsA),
    solutions(PredB, SolnsB),
    ( SolnsA = SolnsB ->
        tree_bitset.member(E, SetA)
    ;
        error("test_bitset: member failed")
    ).

equal(SetA1 - SetB1, SetA2 - SetB2) :-
    ( tree_bitset.equal(SetA1, SetA2) -> EqualA = yes ; EqualA = no),
    ( set_ordlist.equal(SetB1, SetB2) -> EqualB = yes ; EqualB = no),
    ( EqualA = EqualB ->
        EqualA = yes
    ;
        error("test_bitset: equal failed")
    ).

subset(SetA1 - SetB1, SetA2 - SetB2) :-
    ( tree_bitset.subset(SetA1, SetA2) ->
        ( set_ordlist.subset(SetB1, SetB2) ->
            true
        ;
            error("test_bitset: subset succeeded unexpectedly")
        )
    ; set_ordlist.subset(SetB1, SetB2) ->
        error("test_bitset: subset failed unexpectedly")
    ;
        fail
    ).

superset(SetA1 - SetB1, SetA2 - SetB2) :-
    ( tree_bitset.superset(SetA1, SetA2) ->
        ( set_ordlist.superset(SetB1, SetB2) ->
            true
        ;
            error("test_bitset: superset succeeded unexpectedly")
        )
    ; set_ordlist.superset(SetB1, SetB2) ->
        error("test_bitset: superset failed unexpectedly")
    ;
        fail
    ).

%-----------------------------------------------------------------------------%

list_to_set(List) = Result :-
    check0("list_to_set",
        tree_bitset.list_to_set(List) - set_ordlist.list_to_set(List),
        Result).

sorted_list_to_set(List) = Result :-
    check0("sorted_list_to_set",
        tree_bitset.sorted_list_to_set(List) -
            set_ordlist.sorted_list_to_set(List),
        Result).

to_sorted_list(A - B) = List :-
    ListA = tree_bitset.to_sorted_list(A),
    ListB = set_ordlist.to_sorted_list(B),
    ( ListA = ListB ->
        List = ListB
    ;
        error("test_bitset: to_sorted_list failed")
    ).

list_to_set(A, test_bitset.list_to_set(A)).
sorted_list_to_set(A, test_bitset.sorted_list_to_set(A)).
to_sorted_list(A, test_bitset.to_sorted_list(A)).

set_to_bitset(Set) = A - B :-
    set.to_sorted_list(Set, SortedList),
    A - B = test_bitset.sorted_list_to_set(SortedList).

bitset_to_set(A - B) = Set :-
    SortedList = test_bitset.to_sorted_list(A - B),
    set.sorted_list_to_set(SortedList, Set).

from_set(Set) = set_to_bitset(Set).
to_set(Set) = bitset_to_set(Set).

%-----------------------------------------------------------------------------%

insert(E, SetA0 - SetB0, Result) :-
    tree_bitset.insert(E, SetA0, SetA),
    set_ordlist.insert(E, SetB0, SetB),
    check1("insert", SetA0 - SetB0, SetA - SetB, Result).

insert_list(Es, SetA0 - SetB0, Result) :-
    tree_bitset.insert_list(Es, SetA0, SetA),
    set_ordlist.insert_list(Es, SetB0, SetB),
    check1("insert_list", SetA0 - SetB0, SetA - SetB, Result).

delete(E, SetA0 - SetB0, Result) :-
    tree_bitset.delete(E, SetA0, SetA),
    set_ordlist.delete(E, SetB0, SetB),
    check1("delete", SetA0 - SetB0, SetA - SetB, Result).

delete_list(Es, SetA0 - SetB0, Result) :-
    tree_bitset.delete_list(Es, SetA0, SetA),
    set_ordlist.delete_list(Es, SetB0, SetB),
    check1("delete_list", SetA0 - SetB0, SetA - SetB, Result).

remove(E, SetA0 - SetB0, Result) :-
    ( tree_bitset.remove(E, SetA0, SetA1) ->
        ( set_ordlist.remove(E, SetB0, SetB1) ->
            SetA = SetA1,
            SetB = SetB1
        ;
            error("test_bitset: remove succeeded unexpectedly")
        )
    ; set_ordlist.remove(E, SetB0, _) ->
        error("test_bitset: remove failed unexpectedly")
    ;
        fail
    ),
    check1("remove", SetA0 - SetB0, SetA - SetB, Result).

remove_list(Es, SetA0 - SetB0, Result) :-
    ( tree_bitset.remove_list(Es, SetA0, SetA1) ->
        ( set_ordlist.remove_list(Es, SetB0, SetB1) ->
            SetA = SetA1,
            SetB = SetB1
        ;
            error("test_bitset: remove succeeded unexpectedly")
        )
    ; set_ordlist.remove_list(Es, SetB0, _) ->
        error("test_bitset: remove failed unexpectedly")
    ;
        fail
    ),
    check1("remove_list", SetA0 - SetB0, SetA - SetB, Result).

remove_least(Least, SetA0 - SetB0, Result) :-
    ( tree_bitset.remove_least(LeastA, SetA0, SetA1) ->
        ( set_ordlist.remove_least(LeastB, SetB0, SetB1) ->
            ( LeastA = LeastB ->
                Least = LeastA,
                check1("remove_least", SetA0 - SetB0, SetA1 - SetB1, Result)
            ;
                error("test_bitset: remove_least: wrong least element")
            )
        ;
            error("test_bitset: remove_least: should be no least value")
        )
    ; set_ordlist.remove_least(_, SetB0, _) ->
        error("test_bitset: remove_least: failed")
    ;
        fail
    ).

%-----------------------------------------------------------------------------%

union(SetA1 - SetB1, SetA2 - SetB2) = Result :-
    tree_bitset.union(SetA1, SetA2, SetA),
    set_ordlist.union(SetB1, SetB2, SetB),
    check2("union", SetA1 - SetB1, SetA2 - SetB2, SetA - SetB, Result).

union_list(SetsAB) = Result :-
    get_sets("union_list", SetsAB, SetsA, SetsB),
    SetA = tree_bitset.union_list(SetsA),
    SetB = set_ordlist.union_list(SetsB),
    check0("union_list", SetA - SetB, Result).

intersect(SetA1 - SetB1, SetA2 - SetB2) = Result :-
    tree_bitset.intersect(SetA1, SetA2, SetA),
    set_ordlist.intersect(SetB1, SetB2, SetB),
    check2("intersect", SetA1 - SetB1, SetA2 - SetB2, SetA - SetB, Result).

intersect_list(SetsAB) = Result :-
    get_sets("intersect_list", SetsAB, SetsA, SetsB),
    SetA = tree_bitset.intersect_list(SetsA),
    SetB = set_ordlist.intersect_list(SetsB),
    check0("intersect_list", SetA - SetB, Result).

difference(SetA1 - SetB1, SetA2 - SetB2) = Result :-
    tree_bitset.difference(SetA1, SetA2, SetA),
    set_ordlist.difference(SetB1, SetB2, SetB),
    check2("difference", SetA1 - SetB1, SetA2 - SetB2, SetA - SetB, Result).

union(A, B, test_bitset.union(A, B)).
union_list(Sets, test_bitset.union_list(Sets)).
intersect(A, B, test_bitset.intersect(A, B)).
intersect_list(Sets, test_bitset.intersect_list(Sets)).
difference(A, B, test_bitset.difference(A, B)).

:- pred get_sets(string::in, list(pair(tree_bitset(T), set_ordlist(T)))::in,
    list(tree_bitset(T))::out, list(set_ordlist(T))::out) is det <= enum(T).

get_sets(_, [], [], []).
get_sets(Op, [SetA - SetB | SetsAB], [SetA | SetsA], [SetB | SetsB]) :-
    tree_bitset.to_sorted_list(SetA, SetListA),
    set_ordlist.to_sorted_list(SetB, SetListB),
    ( SetListA = SetListB ->
        get_sets(Op, SetsAB, SetsA, SetsB)
    ;
        error("test_bitset: get_sets: unequal sets in " ++ Op ++ " arg list")
    ).

divide(Pred, SetA - SetB, ResultIn, ResultOut) :-
    tree_bitset.divide(Pred, SetA, InSetA, OutSetA),
    set_ordlist.divide(Pred, SetB, InSetB, OutSetB),

    tree_bitset.to_sorted_list(SetA, SetListA),
    set_ordlist.to_sorted_list(SetB, SetListB),
    tree_bitset.to_sorted_list(InSetA, InSetListA),
    set_ordlist.to_sorted_list(InSetB, InSetListB),
    tree_bitset.to_sorted_list(OutSetA, OutSetListA),
    set_ordlist.to_sorted_list(OutSetB, OutSetListB),
    (
        SetListA = SetListB,
        InSetListA = InSetListB,
        OutSetListA = OutSetListB
    ->
        ResultIn = InSetA - InSetB,
        ResultOut = OutSetA - OutSetB
    ;
        error("test_bitset: divide: unequal sets")
    ).

divide_by_set(DivideBySetA - DivideBySetB, SetA - SetB, ResultIn, ResultOut) :-
    tree_bitset.divide_by_set(DivideBySetA, SetA, InSetA, OutSetA),
    set_ordlist.divide_by_set(DivideBySetB, SetB, InSetB, OutSetB),

    tree_bitset.to_sorted_list(DivideBySetA, DivideBySetListA),
    set_ordlist.to_sorted_list(DivideBySetB, DivideBySetListB),
    tree_bitset.to_sorted_list(SetA, SetListA),
    set_ordlist.to_sorted_list(SetB, SetListB),
    tree_bitset.to_sorted_list(InSetA, InSetListA),
    set_ordlist.to_sorted_list(InSetB, InSetListB),
    tree_bitset.to_sorted_list(OutSetA, OutSetListA),
    set_ordlist.to_sorted_list(OutSetB, OutSetListB),
    (
        DivideBySetListA = DivideBySetListB,
        SetListA = SetListB,
        InSetListA = InSetListB,
        OutSetListA = OutSetListB
    ->
        ResultIn = InSetA - InSetB,
        ResultOut = OutSetA - OutSetB
    ;
        error("test_bitset: divide_by_set: unequal sets")
    ).

%-----------------------------------------------------------------------------%

foldl(Pred, SetA - SetB, Acc0, Acc) :-
    tree_bitset.to_sorted_list(SetA, SetListA),
    set_ordlist.to_sorted_list(SetB, SetListB),
    tree_bitset.foldl(Pred, SetA, Acc0, AccA),
    set_ordlist.fold(Pred, SetB, Acc0, AccB),
    ( SetListA = SetListB, AccA = AccB ->
        Acc = AccA
    ;
        error("test_bitset: foldl failed")
    ).

foldl(Pred, SetA - SetB, Acc0) = Acc :-
    tree_bitset.to_sorted_list(SetA, SetListA),
    set_ordlist.to_sorted_list(SetB, SetListB),
    tree_bitset.foldl(Pred, SetA, Acc0) = AccA,
    set_ordlist.fold(Pred, SetB, Acc0) = AccB,
    ( SetListA = SetListB, AccA = AccB ->
        Acc = AccA
    ;
        error("test_bitset: foldl failed")
    ).

filter(Pred, SetA - SetB) = Result :-
    tree_bitset.to_sorted_list(SetA, SetListA),
    set_ordlist.to_sorted_list(SetB, SetListB),
    InSetA = tree_bitset.filter(Pred, SetA),
    InSetB = set_ordlist.filter(Pred, SetB),
    tree_bitset.to_sorted_list(InSetA, InSetListA),
    set_ordlist.to_sorted_list(InSetB, InSetListB),
    ( SetListA = SetListB, InSetListA = InSetListB ->
        Result = InSetA - InSetB
    ;
        error("test_bitset: filter/2 failed")
    ).

filter(Pred, SetA - SetB, ResultIn, ResultOut) :-
    tree_bitset.to_sorted_list(SetA, SetListA),
    set_ordlist.to_sorted_list(SetB, SetListB),
    tree_bitset.filter(Pred, SetA, InSetA, OutSetA),
    set_ordlist.filter(Pred, SetB, InSetB, OutSetB),
    tree_bitset.to_sorted_list(InSetA, InSetListA),
    set_ordlist.to_sorted_list(InSetB, InSetListB),
    tree_bitset.to_sorted_list(OutSetA, OutSetListA),
    set_ordlist.to_sorted_list(OutSetB, OutSetListB),
    ( SetListA = SetListB, InSetListA = InSetListB, OutSetListA = OutSetListB ->
        ResultIn = InSetA - InSetB,
        ResultOut = OutSetA - OutSetB
    ;
        error("test_bitset: filter/4 failed")
    ).

%-----------------------------------------------------------------------------%

:- pred check0(string::in, test_bitset(T)::in, test_bitset(T)::out) is det
    <= enum(T).

check0(Op, Tester, Result) :-
    Tester = BitSet - Set,
    tree_bitset.to_sorted_list(BitSet, BitSetList),
    set_ordlist.to_sorted_list(Set, SetList),
    ( BitSetList = SetList ->
        Result = Tester
    ;
        throw(zero_argument(Op, Tester))
    ).

:- pred check1(string::in, test_bitset(T)::in, test_bitset(T)::in,
    test_bitset(T)::out) is det <= enum(T).

check1(Op, TesterA, Tester, Result) :-
    TesterA = BitSetA - SetA,
    tree_bitset.to_sorted_list(BitSetA, BitSetListA),
    set_ordlist.to_sorted_list(SetA, SetListA),
    Tester = BitSet - Set,
    tree_bitset.to_sorted_list(BitSet, BitSetList),
    set_ordlist.to_sorted_list(Set, SetList),
    ( BitSetListA = SetListA, BitSetList = SetList ->
        Result = Tester
    ;
        throw(one_argument(Op, TesterA, Tester))
    ).

:- pred check2(string::in, test_bitset(T)::in, test_bitset(T)::in,
    test_bitset(T)::in, test_bitset(T)::out) is det <= enum(T).

check2(Op, TesterA, TesterB, Tester, Result) :-
    TesterA = BitSetA - SetA,
    tree_bitset.to_sorted_list(BitSetA, BitSetListA),
    set_ordlist.to_sorted_list(SetA, SetListA),
    TesterB = BitSetB - SetB,
    tree_bitset.to_sorted_list(BitSetB, BitSetListB),
    set_ordlist.to_sorted_list(SetB, SetListB),
    Tester = BitSet - Set,
    tree_bitset.to_sorted_list(BitSet, BitSetList),
    set_ordlist.to_sorted_list(Set, SetList),

    ( BitSetListA = SetListA, BitSetListB = SetListB, BitSetList = SetList ->
        Result = Tester
    ;
        throw(two_arguments(Op, TesterA, TesterB, Tester))
    ).

%-----------------------------------------------------------------------------%
